#include <stdio.h>
#include "water_pump.h"
#include "filesystem.h"
#include "cJSON.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static const char *WATER_PUMP = "WATER_PUMP";

void load_pump_configuration(void)
{
    ESP_LOGI(WATER_PUMP, "========================== Starting Water Pump ==========================");

    const char *default_config = readFile("/spiffs/default.json");

    cJSON *file_json = cJSON_Parse(default_config);

    const cJSON *pump_activation_interval = NULL;

    pump_activation_interval = cJSON_GetObjectItemCaseSensitive(file_json, "pump_activation_interval");

    ESP_LOGW(WATER_PUMP, "%s", default_config);

    if (cJSON_IsString(pump_activation_interval) && (pump_activation_interval->valuestring != NULL))
    {
        ESP_LOGI(WATER_PUMP, "%s", pump_activation_interval->valuestring);
        activate_pump(pump_activation_interval->valuestring, 16);
    }
    else
    {
        ESP_LOGE(WATER_PUMP, "%s", "Not found configuration default");
    }
}

void activate_pump(char *interval, int pump_gpio_num)
{
    int pump_status = 0;
    while (1)
    {

        gpio_set_direction(pump_gpio_num, GPIO_MODE_OUTPUT);

        ESP_LOGI(WATER_PUMP, "Enabling pump!");
        pump_status = !pump_status;
        if (pump_status == 1)
        {
            ESP_LOGI(WATER_PUMP, "Pump Enabled");
        }
        else
        {
            ESP_LOGI(WATER_PUMP, "Pump Disabled");
        }

        gpio_set_level(pump_gpio_num, pump_status);
        vTaskDelay(atoi(interval) / portTICK_PERIOD_MS);
    }
}
