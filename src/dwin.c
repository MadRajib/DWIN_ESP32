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

static const char* TAG = "DWIN";

uint8_t sendbuf[11 + DWIN_WIDTH / 6 * 2] = { 0xAA };
uint8_t tailbuf[4] = { 0xCC, 0x33, 0xC3, 0x3C };
uint8_t recvbuf[26] = { 0 };
uint8_t receivedType;

int recnum = 0;

void DWIN_Byte(size_t *i, const uint16_t bval)
{
  /* 0xAA is already present in send buffer */
  ++(*i);
  sendbuf[*i] = bval;
}

void DWIN_Send(size_t *i)
{
    /* Size of send buffer is byte added + 1
    * since start byte is always 0xAA
    */
    ++(*i);

    /* Send send buffer to uart */
    FOR_L_N(_i, *i) {
        uart_write_bytes(UART_NUM, (const char *)&sendbuf[_i], 1);
        ets_delay_us(1);
        // ESP_LOGW(TAG, "0x%02x ", sendbuf[_i]);
    }
    
    /* Send trailing buffer to uart, always after sending send buffer */
    FOR_L_N(_i, 4) {
        uart_write_bytes(UART_NUM, (const char *)&tailbuf[_i], 1);
        ets_delay_us(1);
        // ESP_LOGW(TAG, "0x%02x ", tailbuf[_i]);
    }

    ESP_ERROR_CHECK(uart_wait_tx_done(UART_NUM, 500));

    /* wait for response */
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

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