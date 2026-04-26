
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "encoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "dwin_pins.h"

#define ENCODER_QUEUE_SIZE 10
#define ENCODER_PULSES_PER_STEP 4

typedef enum {
    ENCODER_EVENT_CW,          // Clockwise rotation
    ENCODER_EVENT_CCW,         // Counter-clockwise rotation
    ENCODER_EVENT_BUTTON_PRESS,
    ENCODER_EVENT_BUTTON_RELEASE,
    ENCODER_EVENT_BUTTON_LONG_PRESS
} encoder_event_type_t;

typedef struct {
    encoder_event_type_t type;
    int32_t position;          // Current encoder position
    uint32_t timestamp;        // Event timestamp in ms
} encoder_event_t;

static const char* TAG = "ENC";
static QueueHandle_t encoder_event_queue = NULL;
static uint8_t previous_encoder_state = 0;

// The Quadrature State Table
// Maps old_state + new_state to a direction: +1 (CW), -1 (CCW), 0 (Invalid/Bounce)
static const int8_t encoder_state_table[16] = {
    0, -1,  1,  0,
    1,  0,  0, -1,
   -1,  0,  0,  1,
    0,  1, -1,  0
};

static void IRAM_ATTR encoder_isr_handler(void* arg) {
    uint8_t a = gpio_get_level(DWIN_ENCODER_CLK);
    uint8_t b = gpio_get_level(DWIN_ENCODER_DT);

    previous_encoder_state <<= 2;
    previous_encoder_state |= (a << 1) | b;
    previous_encoder_state &= 0x0F;

    int8_t movement_delta = encoder_state_table[previous_encoder_state];
    if (movement_delta == 0) {
        return;
    }

    static int8_t step_accumulator = 0;
    step_accumulator += movement_delta;

    if (step_accumulator >= ENCODER_PULSES_PER_STEP || step_accumulator <= -ENCODER_PULSES_PER_STEP) {

        encoder_event_t event;
        event.type = (step_accumulator >= ENCODER_PULSES_PER_STEP) ?
                    ENCODER_EVENT_CW : ENCODER_EVENT_CCW;
        event.position = (step_accumulator >= ENCODER_PULSES_PER_STEP) ?
                    1 : -1;
        event.timestamp = xTaskGetTickCountFromISR();

        step_accumulator = 0;

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(encoder_event_queue, &event, &xHigherPriorityTaskWoken);

        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

void encoder_processing_task(void *pvParameters) {
    encoder_event_t event;
    uint32_t last_event_time = 0;
    int32_t absolute_position = 0;

    while (1) {
        // Sleep here until the ISR pushes a physical click into the queue
        if (xQueueReceive(encoder_event_queue, &event, portMAX_DELAY) == pdPASS) {
            
            int32_t speed_multiplier = 1;

            // CALCULATE SPEED MULTIPLIER (Marlin Logic) ---
            uint32_t time_diff_ms = pdTICKS_TO_MS(event.timestamp - last_event_time);
            last_event_time = event.timestamp;

            if (time_diff_ms > 0 && time_diff_ms < 500) { // If it was a fast click
                float clicks_per_sec = 1000.0f / (float)time_diff_ms;
                
                // Apply your Marlin multipliers safely here in standard code
                if (clicks_per_sec >= 50.0f) {
                    speed_multiplier = 100;
                } else if (clicks_per_sec >= 20.0f) {
                    speed_multiplier = 10;
                } else if (clicks_per_sec >= 10.0f) {
                    speed_multiplier = 5;
                }
            }

            // Multiply the direction (+1 or -1) by the speed multiplier
            int32_t final_movement = event.position * speed_multiplier;
            absolute_position += final_movement;

            ESP_LOGI(TAG, "Moved: %s. Delta: %ld. Abs Pos: %ld", 
                     (event.type == ENCODER_EVENT_CW) ? "CW" : "CCW", 
                     final_movement, 
                     absolute_position);
            
            // update display
        }

    }
}

void encoder_init(void) {

    encoder_event_queue = xQueueCreate(ENCODER_QUEUE_SIZE, sizeof(encoder_event_t));
    if (encoder_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create encoder event queue");
        return;
    }

    gpio_config_t io_conf_a = {
        .pin_bit_mask = (1ULL << DWIN_ENCODER_CLK),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf_a);

    gpio_config_t io_conf_b = {
        .pin_bit_mask = (1ULL << DWIN_ENCODER_DT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf_b);

    gpio_config_t io_conf_btn = {
        .pin_bit_mask = (1ULL << DWIN_ENCODER_BTN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf_btn);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(DWIN_ENCODER_CLK, encoder_isr_handler, NULL);
    gpio_isr_handler_add(DWIN_ENCODER_DT, encoder_isr_handler, NULL);
    // gpio_isr_handler_add(DWIN_ENCODER_BTN, button_isr_handler, NULL);

    xTaskCreate(encoder_processing_task, 
                "enc_update", 
                2048, 
                NULL, 
                10,  // High priority
                NULL);
}