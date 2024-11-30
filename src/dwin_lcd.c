#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include "rom/ets_sys.h"
#include "utils.h"
#include "dwin_lcd.h"

/* DISPLAY size */
#define DWIN_WIDTH 480
#define DWIN_HEIGHT 272

/* UART CONFIG */
#define UART_NUM UART_NUM_2
#define TX_PIN GPIO_NUM_17
#define RX_PIN GPIO_NUM_16
#define UART_BAUDRATE 115200
#define UART_READ_TIMEOUT_MS 1000

static const char *TAG = "DWIN";

uint8_t sendbuf[11 + DWIN_WIDTH / 6 * 2] = {0xAA};
uint8_t tailbuf[4] = {0xCC, 0x33, 0xC3, 0x3C};
uint8_t recvbuf[26] = {0};
uint8_t receivedType;

int recnum = 0;

void DWIN_Byte(size_t *i, const uint16_t bval)
{
    /* 0xAA is already present in send buffer */
    ++(*i);
    sendbuf[*i] = bval;
}

inline void DWIN_Word(size_t *i, const uint16_t wval) {
    ++(*i);
    sendbuf[*i] = wval >> 8;
    ++(*i);
    sendbuf[*i] = wval & 0xFF;
}

void DWIN_Send(size_t *i)
{
    /* Size of send buffer is byte added + 1
     * since start byte is always 0xAA
     */
    ++(*i);

    /* Send send buffer to uart */
    LOOP_L_N(_i, *i)
    {
        uart_write_bytes(UART_NUM, (const char *)&sendbuf[_i], 1);
        ets_delay_us(1);
        // ESP_LOGW(TAG, "0x%02x ", sendbuf[_i]);
    }

    /* Send trailing buffer to uart, always after sending send buffer */
    LOOP_L_N(_i, 4)
    {
        uart_write_bytes(UART_NUM, (const char *)&tailbuf[_i], 1);
        ets_delay_us(1);
        // ESP_LOGW(TAG, "0x%02x ", tailbuf[_i]);
    }

    ESP_ERROR_CHECK(uart_wait_tx_done(UART_NUM, 500));

    /* wait for response */
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

inline void DWIN_Long(size_t *i, const uint32_t lval)
{
    ++(*i);
    sendbuf[*i] = (lval >> 24) & 0xFF;
    ++(*i);
    sendbuf[*i] = (lval >> 16) & 0xFF;
    ++(*i);
    sendbuf[*i] = (lval >> 8) & 0xFF;
    ++(*i);
    sendbuf[*i] = lval & 0xFF;
}

inline void DWIN_String(size_t *i, const char *string)
{
    const size_t len = _MIN(sizeof(sendbuf) - (*i), strlen(string));
    if (len == 0) return;
    memcpy(&sendbuf[*i + 1], string, len);
    *i += len;
}

/*-------------------------------------- System variable function --------------------------------------*/

// Handshake (1: Success, 0: Fail)
bool DWIN_Handshake(void)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x00);
    DWIN_Send(&i);
    
    size_t buffered_len = 0;

    while (1) {
        buffered_len = 0;
        uart_get_buffered_data_len(UART_NUM, &buffered_len);

        if (buffered_len > 0 && recnum < (signed)sizeof(recvbuf)) {

            // Read data from UART
            int len = uart_read_bytes(UART_NUM, &recvbuf[recnum], 1, UART_READ_TIMEOUT_MS / portTICK_PERIOD_MS);
            
            if (len > 0) {
                // Check for valid data
                if (recvbuf[0] != 0xAA) { // Invalid data check
                    if (recnum > 0) {
                        recnum = 0; // Reset if invalid data
                        memset(recvbuf, 0, sizeof(recvbuf));
                    }
                    continue;
                }
                vTaskDelay(10 / portTICK_PERIOD_MS); // Delay for 10ms to simulate the `delay(10)`
                recnum++; // Increment the buffer index
            }
        } else {
            break;
        }
    }

    return ( recnum >= 3
        && recvbuf[0] == 0xAA
        && recvbuf[1] == '\0'
        && recvbuf[2] == 'O'
        && recvbuf[3] == 'K' );
}

// Set the backlight luminance
//  luminance: (0x00-0xFF)
void DWIN_Backlight_SetLuminance(const uint8_t luminance)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x30);
    DWIN_Byte(&i, _MAX(luminance, 0x1F));
    DWIN_Send(&i);
}

// Set screen display direction
//  dir: 0=0°, 1=90°, 2=180°, 3=270°
void DWIN_Frame_SetDir(uint8_t dir)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x34);
    DWIN_Byte(&i, 0x5A);
    DWIN_Byte(&i, 0xA5);
    DWIN_Byte(&i, dir);
    DWIN_Send(&i);
}

// Update display
void DWIN_UpdateLCD(void)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x3D);
    DWIN_Send(&i);
}

/*---------------------------------------- Drawing functions ----------------------------------------*/

// Clear screen
//  color: Clear screen color
void DWIN_Frame_Clear(const uint16_t color)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x01);
    DWIN_Word(&i, color);
    DWIN_Send(&i);
}

// Draw a point
//  width: point width   0x01-0x0F
//  height: point height 0x01-0x0F
//  x,y: upper left point
void DWIN_Draw_Point(uint8_t width, uint8_t height, uint16_t x, uint16_t y)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x02);
    DWIN_Byte(&i, width);
    DWIN_Byte(&i, height);
    DWIN_Word(&i, x);
    DWIN_Word(&i, y);
    DWIN_Send(&i);
}

// Draw a line
//  color: Line segment color
//  xStart/yStart: Start point
//  xEnd/yEnd: End point
void DWIN_Draw_Line(uint16_t color, uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x03);
    DWIN_Word(&i, color);
    DWIN_Word(&i, xStart);
    DWIN_Word(&i, yStart);
    DWIN_Word(&i, xEnd);
    DWIN_Word(&i, yEnd);
    DWIN_Send(&i);
}

// Draw a rectangle
//  mode: 0=frame, 1=fill, 2=XOR fill
//  color: Rectangle color
//  xStart/yStart: upper left point
//  xEnd/yEnd: lower right point
void DWIN_Draw_Rectangle(uint8_t mode, uint16_t color,
                         uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x05);
    DWIN_Byte(&i, mode);
    DWIN_Word(&i, color);
    DWIN_Word(&i, xStart);
    DWIN_Word(&i, yStart);
    DWIN_Word(&i, xEnd);
    DWIN_Word(&i, yEnd);
    DWIN_Send(&i);
}

// Move a screen area
//  mode: 0, circle shift; 1, translation
//  dir: 0=left, 1=right, 2=up, 3=down
//  dis: Distance
//  color: Fill color
//  xStart/yStart: upper left point
//  xEnd/yEnd: bottom right point
void DWIN_Frame_AreaMove(uint8_t mode, uint8_t dir, uint16_t dis,
                         uint16_t color, uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x09);
    DWIN_Byte(&i, (mode << 7) | dir);
    DWIN_Word(&i, dis);
    DWIN_Word(&i, color);
    DWIN_Word(&i, xStart);
    DWIN_Word(&i, yStart);
    DWIN_Word(&i, xEnd);
    DWIN_Word(&i, yEnd);
    DWIN_Send(&i);
}

/*---------------------------------------- Text related functions ----------------------------------------*/

// Draw a string
//  widthAdjust: true=self-adjust character width; false=no adjustment
//  bShow: true=display background color; false=don't display background color
//  size: Font size
//  color: Character color
//  bColor: Background color
//  x/y: Upper-left coordinate of the string
//  *string: The string
void DWIN_Draw_String(bool widthAdjust, bool bShow, uint8_t size,
                      uint16_t color, uint16_t bColor, uint16_t x, uint16_t y, const char *string)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x11);
    // Bit 7: widthAdjust
    // Bit 6: bShow
    // Bit 5-4: Unused (0)
    // Bit 3-0: size
    DWIN_Byte(&i, (widthAdjust * 0x80) | (bShow * 0x40) | size);
    DWIN_Word(&i, color);
    DWIN_Word(&i, bColor);
    DWIN_Word(&i, x);
    DWIN_Word(&i, y);
    DWIN_String(&i, string);
    DWIN_Send(&i);
}

// Draw a positive integer
//  bShow: true=display background color; false=don't display background color
//  zeroFill: true=zero fill; false=no zero fill
//  zeroMode: 1=leading 0 displayed as 0; 0=leading 0 displayed as a space
//  size: Font size
//  color: Character color
//  bColor: Background color
//  iNum: Number of digits
//  x/y: Upper-left coordinate
//  value: Integer value
void DWIN_Draw_IntValue(uint8_t bShow, bool zeroFill, uint8_t zeroMode, uint8_t size, uint16_t color,
                        uint16_t bColor, uint8_t iNum, uint16_t x, uint16_t y, uint16_t value)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x14);
    // Bit 7: bshow
    // Bit 6: 1 = signed; 0 = unsigned number;
    // Bit 5: zeroFill
    // Bit 4: zeroMode
    // Bit 3-0: size
    DWIN_Byte(&i, (bShow * 0x80) | (zeroFill * 0x20) | (zeroMode * 0x10) | size);
    DWIN_Word(&i, color);
    DWIN_Word(&i, bColor);
    DWIN_Byte(&i, iNum);
    DWIN_Byte(&i, 0); // fNum
    DWIN_Word(&i, x);
    DWIN_Word(&i, y);
#if 0
    for (char count = 0; count < 8; count++) {
     DWIN_Byte(&i, value);
      value >>= 8;
      if (!(value & 0xFF)) break;
    }
#else
    // Write a big-endian 64 bit integer
    const size_t p = i + 1;
    for (char count = 8; count--;)
    { // 7..0
        ++i;
        sendbuf[p + count] = value;
        value >>= 8;
    }
#endif

    DWIN_Send(&i);
}

// Draw a floating point number
//  bShow: true=display background color; false=don't display background color
//  zeroFill: true=zero fill; false=no zero fill
//  zeroMode: 1=leading 0 displayed as 0; 0=leading 0 displayed as a space
//  size: Font size
//  color: Character color
//  bColor: Background color
//  iNum: Number of whole digits
//  fNum: Number of decimal digits
//  x/y: Upper-left point
//  value: Float value
void DWIN_Draw_FloatValue(uint8_t bShow, bool zeroFill, uint8_t zeroMode, uint8_t size, uint16_t color,
                          uint16_t bColor, uint8_t iNum, uint8_t fNum, uint16_t x, uint16_t y, long value)
{
    // uint8_t *fvalue = (uint8_t*)&value;
    size_t i = 0;
    DWIN_Byte(&i, 0x14);
    DWIN_Byte(&i, (bShow * 0x80) | (zeroFill * 0x20) | (zeroMode * 0x10) | size);
    DWIN_Word(&i, color);
    DWIN_Word(&i, bColor);
    DWIN_Byte(&i, iNum);
    DWIN_Byte(&i, fNum);
    DWIN_Word(&i, x);
    DWIN_Word(&i, y);
    DWIN_Long(&i, value);
    /*
   DWIN_Byte(&i, fvalue[3]);
   DWIN_Byte(&i, fvalue[2]);
   DWIN_Byte(&i, fvalue[1]);
   DWIN_Byte(&i, fvalue[0]);
    */
    DWIN_Send(&i);
}

/*---------------------------------------- Picture related functions ----------------------------------------*/

// Draw JPG and cached in #0 virtual display area
// id: Picture ID
void DWIN_JPG_ShowAndCache(const uint8_t id)
{
    size_t i = 0;
    DWIN_Word(&i, 0x2200);
    DWIN_Byte(&i, id);
    DWIN_Send(&i); // AA 23 00 00 00 00 08 00 01 02 03 CC 33 C3 3C
}

// Draw an Icon
//  libID: Icon library ID
//  picID: Icon ID
//  x/y: Upper-left point
void DWIN_ICON_Show(uint8_t libID, uint8_t picID, uint16_t x, uint16_t y)
{
    NOMORE(x, DWIN_WIDTH - 1);
    NOMORE(y, DWIN_HEIGHT - 1); // -- ozy -- srl
    size_t i = 0;
    DWIN_Byte(&i, 0x23);
    DWIN_Word(&i, x);
    DWIN_Word(&i, y);
    DWIN_Byte(&i, 0x80 | libID);
    DWIN_Byte(&i, picID);
    DWIN_Send(&i);
}

// Unzip the JPG picture to a virtual display area
//  n: Cache index
//  id: Picture ID
void DWIN_JPG_CacheToN(uint8_t n, uint8_t id)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x25);
    DWIN_Byte(&i, n);
    DWIN_Byte(&i, id);
    DWIN_Send(&i);
}

// Copy area from virtual display area to current screen
//  cacheID: virtual area number
//  xStart/yStart: Upper-left of virtual area
//  xEnd/yEnd: Lower-right of virtual area
//  x/y: Screen paste point
void DWIN_Frame_AreaCopy(uint8_t cacheID, uint16_t xStart, uint16_t yStart,
                         uint16_t xEnd, uint16_t yEnd, uint16_t x, uint16_t y)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x27);
    DWIN_Byte(&i, 0x80 | cacheID);
    DWIN_Word(&i, xStart);
    DWIN_Word(&i, yStart);
    DWIN_Word(&i, xEnd);
    DWIN_Word(&i, yEnd);
    DWIN_Word(&i, x);
    DWIN_Word(&i, y);
    DWIN_Send(&i);
}

// Animate a series of icons
//  animID: Animation ID; 0x00-0x0F
//  animate: true on; false off;
//  libID: Icon library ID
//  picIDs: Icon starting ID
//  picIDe: Icon ending ID
//  x/y: Upper-left point
//  interval: Display time interval, unit 10mS
void DWIN_ICON_Animation(uint8_t animID, bool animate, uint8_t libID, uint8_t picIDs, uint8_t picIDe, uint16_t x, uint16_t y, uint16_t interval)
{
    NOMORE(x, DWIN_WIDTH - 1);
    NOMORE(y, DWIN_HEIGHT - 1); // -- ozy -- srl
    size_t i = 0;
    DWIN_Byte(&i, 0x28);
    DWIN_Word(&i, x);
    DWIN_Word(&i, y);
    // Bit 7: animation on or off
    // Bit 6: start from begin or end
    // Bit 5-4: unused (0)
    // Bit 3-0: animID
    DWIN_Byte(&i, (animate * 0x80) | 0x40 | animID);
    DWIN_Byte(&i, libID);
    DWIN_Byte(&i, picIDs);
    DWIN_Byte(&i, picIDe);
    DWIN_Byte(&i, interval);
    DWIN_Send(&i);
}

// Animation Control
//  state: 16 bits, each bit is the state of an animation id
void DWIN_ICON_AnimationControl(uint16_t state)
{
    size_t i = 0;
    DWIN_Byte(&i, 0x28);
    DWIN_Word(&i, state);
    DWIN_Send(&i);
}

/*---------------------------------------- Memory functions ----------------------------------------*/
// The LCD has an additional 32KB SRAM and 16KB Flash

// Data can be written to the sram and save to one of the jpeg page files

// Write Data Memory
//  command 0x31
//  Type: Write memory selection; 0x5A=SRAM; 0xA5=Flash
//  Address: Write data memory address; 0x000-0x7FFF for SRAM; 0x000-0x3FFF for Flash
//  Data: data
//
//  Flash writing returns 0xA5 0x4F 0x4B

// Read Data Memory
//  command 0x32
//  Type: Read memory selection; 0x5A=SRAM; 0xA5=Flash
//  Address: Read data memory address; 0x000-0x7FFF for SRAM; 0x000-0x3FFF for Flash
//  Length: leangth of data to read; 0x01-0xF0
//
//  Response:
//    Type, Address, Length, Data

// Write Picture Memory
//  Write the contents of the 32KB SRAM data memory into the designated image memory space
//  Issued: 0x5A, 0xA5, PIC_ID
//  Response: 0xA5 0x4F 0x4B
//
//  command 0x33
//  0x5A, 0xA5
//  PicId: Picture Memory location, 0x00-0x0F
//
//  Flash writing returns 0xA5 0x4F 0x4B

int DWIN_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl =  UART_HW_FLOWCTRL_DISABLE,
    };

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    // Set UART pins
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    // Install UART driver
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, 1024, 0, 0, NULL, 0));

    return 0;
}