#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include "rom/ets_sys.h"
#include "utils.h"
#include "dwin.h"

/* DISPLAY size */
#define DWIN_WIDTH  480 
#define DWIN_HEIGHT 272

/* UART CONFIG */
#define UART_NUM UART_NUM_2
#define TX_PIN GPIO_NUM_17
#define RX_PIN GPIO_NUM_16
#define UART_BAUDRATE 115200
#define UART_READ_TIMEOUT_MS 1000

#define _MIN(l, r) ((l < r)? l: r)

static const char* TAG = "DWIN";

static uint8_t send_buf[11 + DWIN_WIDTH / 6 * 2] = { 0xAA };
static uint8_t send_buf_tail[4] = { 0xCC, 0x33, 0xC3, 0x3C };
static uint8_t read_buf[26] = { 0 };
static size_t buf_index = 1;

int dwin_init(void)
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

bool dwin_handshake(void)
{
    dwin_add_byte(0x00);
    dwin_send();
    ets_delay_us(10000);
    
    size_t buffered_len = 0;
    int recnum = 0;

    while (1) {
        buffered_len = 0;
        uart_get_buffered_data_len(UART_NUM, &buffered_len);

        if (buffered_len > 0 && recnum < (signed)sizeof(read_buf)) {

            // Read data from UART
            int len = uart_read_bytes(UART_NUM, &read_buf[recnum], 1, UART_READ_TIMEOUT_MS / portTICK_PERIOD_MS);
            
            if (len > 0) {
                // Check for valid data
                if (read_buf[0] != 0xAA) { // Invalid data check
                    if (recnum > 0) {
                        recnum = 0; // Reset if invalid data
                        memset(read_buf, 0, sizeof(read_buf));
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
        && read_buf[0] == 0xAA
        && read_buf[1] == '\0'
        && read_buf[2] == 'O'
        && read_buf[3] == 'K' );
}

/* Send data buf to display buffer */
void dwin_send(void) {

  for (int i = 0; i < buf_index; i++) {
    uart_write_bytes(UART_NUM, (const char *)&send_buf[i], 1);
    ets_delay_us(1);
  }

  for (int i = 0; i < 4; i++) {
    uart_write_bytes(UART_NUM, (const char *)&send_buf_tail[i], 1);
    ets_delay_us(1);
  }

  /* Reset buffer index */
  buf_index = 1;
}

void dwin_update_lcd(void)
{
  dwin_add_byte(0x3D);
  dwin_send();
  ets_delay_us(10000);
}

void dwin_add_byte(uint8_t bval) {
  send_buf[buf_index++] = bval;
}

void dwin_add_word(uint16_t bval) {
  send_buf[buf_index++] = bval >> 8;
  send_buf[buf_index++] = bval & 0xFF;
}

void dwin_add_long(uint32_t lval) {
  send_buf[buf_index++] = (lval >> 24) & 0xFF;
  send_buf[buf_index++] = (lval >> 16) & 0xFF;
  send_buf[buf_index++] = (lval >>  8) & 0xFF;
  send_buf[buf_index++] = lval & 0xFF;
}

void dwin_add_string(char * const string) {
  const size_t len = _MIN(sizeof(send_buf) - buf_index, strlen(string)) + 1;
  memcpy(&send_buf[buf_index], string, len);
  buf_index += len;
}


void dwin_write_big_endian_int64(uint16_t value) {
  // Write a big-endian 64 bit integer
  const size_t p = buf_index;
  for (char count = 8; count--;) { // 7..0
    ++buf_index;
    send_buf[p + count] = value;
    value >>= 8;
  }
}