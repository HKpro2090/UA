#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "soc/uart_channel.h"
#include "esp_log.h"
#define EXAMPLE_UART_NUM 0
#define EXAMPLE_UART_TX_IO_NUM UART_NUM_0_TXD_DIRECT_GPIO_NUM
#define EXAMPLE_UART_RX_IO_NUM UART_NUM_0_RXD_DIRECT_GPIO_NUM

#define EXAMPLE_UART_WAKEUP_THRESHOLD 3

static const char *TAG = "uart_wakeup";

static QueueHandle_t uart_evt_que = NULL;

static void uart_wakeup_task(void *arg)
{
    uart_event_t event;
    while (1)
    {
        
        if (xQueueReceive(uart_evt_que, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            switch (event.type)
            {
            case UART_DATA_BREAK:
                ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                char *msg = "Random Message";
                uart_write_bytes(UART_NUM_0, (const char *)msg, strlen(msg));
                break;
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
    vTaskDelete(NULL);
}

static void light_sleep_task(void *args)
{
    while (1)
    {
        printf("Entering light sleep\n");
        uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);
        esp_light_sleep_start();
        // Sleep fuction not working dure ESP-IDF 4.3.2. Requires v4.4 or above
        // https://github.com/espressif/arduino-esp32/issues/5107
}
    vTaskDelete(NULL);
}

void app_main()
{
    ESP_LOGI(TAG, "Hello");
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        // disabling hw flowctl works in usbUART otherwise the board resets
        //.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_REF_TICK,

    };
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, UART_FIFO_LEN * 2, UART_FIFO_LEN * 2, 20, &uart_evt_que, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, EXAMPLE_UART_TX_IO_NUM, EXAMPLE_UART_RX_IO_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(gpio_sleep_set_direction(EXAMPLE_UART_RX_IO_NUM, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_sleep_set_pull_mode(EXAMPLE_UART_RX_IO_NUM, GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(uart_set_wakeup_threshold(UART_NUM_0, EXAMPLE_UART_WAKEUP_THRESHOLD));
    ESP_ERROR_CHECK(esp_sleep_enable_uart_wakeup(UART_NUM_0));
    xTaskCreate(uart_wakeup_task, "uart_wakeup_task", 4096, NULL, 5, NULL);
}