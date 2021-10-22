#include <stdio.h>
#include "water_pump.h"
#include "filesystem.h"
#include "cJSON.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "time.h"
#include <string.h>

static const char *WATER_PUMP = "WATER_PUMP";
#define PUMP_GPIO 18

void pump_actions()
{
    cJSON *pump_config_t = NULL;
    int pump_activation_interval = 0;
    char *pump_config_json = NULL;
    cJSON *pump_config_str_interval = NULL;
    pump_config_json = readFile("/spiffs/pump/config.json");
    pump_config_t = cJSON_Parse(pump_config_json);

    pump_config_str_interval = cJSON_GetObjectItemCaseSensitive(pump_config_t, "interval");

    if (cJSON_IsString(pump_config_str_interval) && (pump_config_str_interval->valuestring != NULL))
    {
        pump_config_json = pump_config_str_interval->valuestring;
        pump_activation_interval = atoi(pump_config_json);
    }

    ESP_LOGI(WATER_PUMP, "Setting gpio of pump...");
    gpio_pad_select_gpio(PUMP_GPIO);
    gpio_set_direction(PUMP_GPIO, GPIO_MODE_OUTPUT);
    activate_pump(1, PUMP_GPIO);
    vTaskDelay(pump_activation_interval / portTICK_PERIOD_MS);
    activate_pump(0, PUMP_GPIO);
    pump_config_json = NULL;
    pump_config_str_interval = NULL;
}

void activate_pump(int pump_status, int pump_gpio_num)
{

    ESP_LOGI(WATER_PUMP, "Enabling pump!");

    if (pump_status == 1)
    {
        gpio_set_level(pump_gpio_num, pump_gpio_num);
        ESP_LOGI(WATER_PUMP, "Pump Enabled");
    }
    else
    {
        gpio_set_level(pump_gpio_num, pump_gpio_num);
        ESP_LOGI(WATER_PUMP, "Pump Disabled");
    }
}
