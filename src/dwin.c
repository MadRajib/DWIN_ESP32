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

#define SEND_BUF_START_INDEX 1
// static const char* TAG = "DWIN";

uint8_t sendbuf[11 + DWIN_WIDTH / 6 * 2] = { 0xAA };
uint8_t tailbuf[4] = { 0xCC, 0x33, 0xC3, 0x3C };
uint8_t recvbuf[26] = { 0 };
uint8_t receivedType;
static size_t sbuf_indx = SEND_BUF_START_INDEX;

int recnum = 0;

void DWIN_Byte(const uint8_t bval)
{
    sendbuf[sbuf_indx++] = bval;
}

void DWIN_Send(void)
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

bool DWIN_Handshake(void)
{
    DWIN_Byte(0x00);
    DWIN_Send();
    
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