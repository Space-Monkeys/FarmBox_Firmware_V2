#include <stdio.h>
#include "water_pump.h"
#include "filesystem.h"
#include "cJSON.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "time.h"

static const char *WATER_PUMP = "WATER_PUMP";

void load_pump_configuration(time_t now)
{
    ESP_LOGI(WATER_PUMP, "========================== Starting Water Pump ==========================");

    long int number_atv_next;
    long int active_pump_time;
    char next_pump_activation[20];
    char last_pump_activation[20];
    char *next_time_to_action;
    char *pump_activation_interval;
    char *actions_string;
    char *actions;
    cJSON *update_config = NULL;

    actions = readFile("/spiffs/config/pump/actions.json");
    cJSON *actions_json = cJSON_Parse(actions);

    next_time_to_action = cJSON_GetObjectItemCaseSensitive(actions_json, "next_pump_activation")->valuestring;

    int number_atv = atoi(next_time_to_action);

    char *active_pump_time_json = cJSON_GetObjectItemCaseSensitive(actions_json, "active_pump_time")->valuestring;

    active_pump_time = atoi(active_pump_time_json);

    update_config = cJSON_GetObjectItemCaseSensitive(actions_json, "update_config");

    if (cJSON_IsString(update_config) && (update_config->valuestring != NULL))
    {
        char action_char = *update_config->valuestring;
        char u = *"U";

        if (action_char == u)
        {

            char *default_config = readFile("/spiffs/default.json");
            cJSON *default_config_json = cJSON_Parse(default_config);

            active_pump_time_json = cJSON_GetObjectItemCaseSensitive(default_config_json, "active_pump_time")->valuestring;
            pump_activation_interval = cJSON_GetObjectItemCaseSensitive(default_config_json, "pump_activation_interval")->valuestring;

            number_atv = atoi(pump_activation_interval) + now;
            sprintf(next_pump_activation, "%d", number_atv);

            cJSON_GetObjectItemCaseSensitive(actions_json, "next_pump_activation")->valuestring = next_pump_activation;
            cJSON_GetObjectItemCaseSensitive(actions_json, "update_config")->valuestring = "N";
            cJSON_GetObjectItemCaseSensitive(actions_json, "pump_interval")->valuestring = pump_activation_interval;
            cJSON_GetObjectItemCaseSensitive(actions_json, "active_pump_time")->valuestring = active_pump_time_json;

            actions_string = cJSON_Print(actions_json);
            writeFile("/spiffs/config/pump/actions.json", actions_string);

            ESP_LOGI(WATER_PUMP, "%s", actions_string);
            cJSON_Delete(default_config_json);
        }
    }
    ESP_LOGI(WATER_PUMP, "Now is:%ld \n", now);
    ESP_LOGI(WATER_PUMP, "Next Activation is %d \n", number_atv);

    if (now >= number_atv)
    {
        ESP_LOGI(WATER_PUMP, "Opa vou ativar agora");

        pump_activation_interval = cJSON_GetObjectItemCaseSensitive(actions_json, "pump_interval")->valuestring;
        number_atv_next = atoi(pump_activation_interval) + now + active_pump_time;

        gpio_set_direction(12, GPIO_MODE_OUTPUT);
        gpio_set_level(12, 1);
        ESP_LOGI(WATER_PUMP, "Bomba ativada");
        vTaskDelay(active_pump_time / portTICK_PERIOD_MS);
        gpio_set_level(12, 0);
        ESP_LOGI(WATER_PUMP, "Bomba Desativada");

        sprintf(next_pump_activation, "%ld", number_atv_next);
        sprintf(last_pump_activation, "%ld", now);

        cJSON_GetObjectItemCaseSensitive(actions_json, "next_pump_activation")->valuestring = next_pump_activation;
        cJSON_GetObjectItemCaseSensitive(actions_json, "last_pump_activation")->valuestring = last_pump_activation;
        actions_string = cJSON_Print(actions_json);
        writeFile("/spiffs/config/pump/actions.json", actions_string);
    }
    else
    {
        ESP_LOGI(WATER_PUMP, "Opa vou ativar depois");
    }
}

/* void activate_pump(int pump_status, int pump_gpio_num)
{

  

    ESP_LOGI(WATER_PUMP, "Enabling pump!");

    if (pump_status == 1)
    {
        ESP_LOGI(WATER_PUMP, "Pump Enabled");
        
        
    }
    else
    {
        ESP_LOGI(WATER_PUMP, "Pump Disabled");
         gpio_set_level(pump_gpio_num, );
    }
   
} */
