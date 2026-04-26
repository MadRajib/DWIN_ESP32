#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include "rom/ets_sys.h"
#include "utils.h"
#include "dwin.h"
#include "dwin_pins.h"
#include "dwin_pins.h"

#define SEND_BUF_START_INDEX 1
// static const char* TAG = "DWIN";

uint8_t sendbuf[11 + DWIN_WIDTH / 6 * 2] = { 0xAA };
uint8_t tailbuf[4] = { 0xCC, 0x33, 0xC3, 0x3C };
uint8_t recvbuf[26] = { 0 };
uint8_t receivedType;
static size_t sbuf_indx = SEND_BUF_START_INDEX;

int recnum = 0;

void DWIN_add_byte(const uint8_t bval)
{
    sendbuf[sbuf_indx++] = bval;
}

void DWIN_add_word(const uint16_t bval)
{
    sendbuf[sbuf_indx++] = bval >> 8;
    sendbuf[sbuf_indx++] = bval & 0xFF;
}

void DWIN_add_long(const uint32_t lval)
{
    sendbuf[sbuf_indx++] = (lval >> 24) & 0xFF;
    sendbuf[sbuf_indx++] = (lval >> 16) & 0xFF;
    sendbuf[sbuf_indx++] = (lval >>  8) & 0xFF;
    sendbuf[sbuf_indx++] = lval & 0xFF;
}

void DWIN_add_string(const char * const string)
{
    const size_t len = _MIN(sizeof(sendbuf) - sbuf_indx, strlen(string)) + 1;
    memcpy(&sendbuf[sbuf_indx], string, len);
    sbuf_indx += len;
}

/*
  Draw an Icon
  libID: Icon library ID
  picID: Icon ID
  x/y: Upper-left point
*/
void DWIN_add_icon(uint8_t libID, uint8_t picID, uint16_t x, uint16_t y)
{  
    x = _MIN(x, DWIN_WIDTH - 1);
    y = _MIN(y, DWIN_HEIGHT - 1);
    DWIN_add_byte(0x23);
    DWIN_add_word(x);
    DWIN_add_word(y);
    DWIN_add_byte(0x80 | libID);
    DWIN_add_byte(picID); 
    DWIN_send();
}

/*
  Draw a rectangle
  mode: 0=frame, 1=fill, 2=XOR fill
  color: Rectangle color
  xStart/yStart: upper left point
  xEnd/yEnd: lower right point
*/
void DWIN_add_rect(uint8_t mode, uint16_t color,
                         uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
    DWIN_add_byte(0x05);
    DWIN_add_byte(mode);
    DWIN_add_word(color);
    DWIN_add_word(xStart);
    DWIN_add_word(yStart);
    DWIN_add_word(xEnd);
    DWIN_add_word(yEnd);
    DWIN_send();
}

/*
 Draw a string
  widthAdjust: true=self-adjust character width; false=no adjustment
  bShow: true=display background color; false=don't display background color
  size: Font size
  color: Character color
  bColor: Background color
  x/y: Upper-left coordinate of the string
  *string: The string
*/
void DWIN_draw_string(bool widthAdjust, bool bShow, uint8_t size,
                      uint16_t color, uint16_t bColor, uint16_t x, uint16_t y, char *string)
{
    DWIN_add_byte(0x11);
    /*
    Bit 7: widthAdjust
    Bit 6: bShow
    Bit 5-4: Unused (0)
    Bit 3-0: size
    */
    DWIN_add_byte((widthAdjust * 0x80) | (bShow * 0x40) | size);
    DWIN_add_word(color);
    DWIN_add_word(bColor);
    DWIN_add_word(x);
    DWIN_add_word(y);
    DWIN_add_string(string);
    DWIN_send();
}

/*
 Draw a positive integer
  bShow: true=display background color; false=don't display background color
  zeroFill: true=zero fill; false=no zero fill
  zeroMode: 1=leading 0 displayed as 0; 0=leading 0 displayed as a space
  size: Font size
  color: Character color
  bColor: Background color
  iNum: Number of digits
  x/y: Upper-left coordinate
  value: Integer value
*/
void DWIN_draw_int(uint8_t bShow, bool zeroFill, uint8_t zeroMode, uint8_t size, uint16_t color,
                          uint16_t bColor, uint8_t iNum, uint16_t x, uint16_t y, uint16_t value)
{
    DWIN_add_byte(0x14);
    // Bit 7: bshow
    // Bit 6: 1 = signed; 0 = unsigned number;
    // Bit 5: zeroFill
    // Bit 4: zeroMode
    // Bit 3-0: size
    DWIN_add_byte((bShow * 0x80) | (zeroFill * 0x20) | (zeroMode * 0x10) | size);
    DWIN_add_word(color);
    DWIN_add_word(bColor);
    DWIN_add_byte(iNum);
    DWIN_add_byte(0); // fNum
    DWIN_add_word(x);
    DWIN_add_word(y);

    // Write a big-endian 64 bit integer
    const size_t p = sbuf_indx;
    for (char count = 8; count--;) { // 7..0
        ++sbuf_indx;
        sendbuf[p + count] = value;
        value >>= 8;
    }

    DWIN_send();
}


void DWIN_send(void)
{
    /* Send send buffer to uart */
    FOR_L_N(_i, sbuf_indx) {
        uart_write_bytes(DWIN_UART_NUM, (const char *)&sendbuf[_i], 1);
        ets_delay_us(1);
        // ESP_LOGW(TAG, "0x%02x ", sendbuf[_i]);
    }
    
    /* Send trailing buffer to uart, always after sending send buffer */
    FOR_L_N(_i, 4) {
        uart_write_bytes(DWIN_UART_NUM, (const char *)&tailbuf[_i], 1);
        ets_delay_us(1);
        // ESP_LOGW(TAG, "0x%02x ", tailbuf[_i]);
    }

    ESP_ERROR_CHECK(uart_wait_tx_done(DWIN_UART_NUM, 500));

    /* wait for response */
    vTaskDelay(10 / portTICK_PERIOD_MS);

    sbuf_indx = SEND_BUF_START_INDEX;
}

bool DWIN_handshake(void)
{
    DWIN_add_byte(0x00);
    DWIN_send();
    
    size_t buffered_len = 0;

    while (1) {
        buffered_len = 0;
        uart_get_buffered_data_len(DWIN_UART_NUM, &buffered_len);

        if (buffered_len > 0 && recnum < (signed)sizeof(recvbuf)) {

            // Read data from UART
            int len = uart_read_bytes(DWIN_UART_NUM, &recvbuf[recnum], 1, DIWN_UART_READ_TIMEOUT_MS / portTICK_PERIOD_MS);
            
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

int DWIN_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = DWIN_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl =  UART_HW_FLOWCTRL_DISABLE,
    };

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(DWIN_UART_NUM, &uart_config));
    // Set UART pins
    ESP_ERROR_CHECK(uart_set_pin(DWIN_UART_NUM, DWIN_UART_TX, DWIN_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    // Install UART driver
    ESP_ERROR_CHECK(uart_driver_install(DWIN_UART_NUM, 1024, 0, 0, NULL, 0));

    return 0;
}