#include <stdio.h>
#include "lights.h"
#include "filesystem.h"
#include "driver/gpio.h"
#include "cJSON.h"
#include "esp_log.h"

#define LIGHTS_GPIO 19
static const char *LIGHTS = "LIGHTS";
void light_actions(void)
{

    ESP_LOGI(LIGHTS, "Setting gpio of lights...");
    gpio_pad_select_gpio(LIGHTS_GPIO);
    gpio_set_direction(LIGHTS_GPIO, GPIO_MODE_OUTPUT);

    cJSON *lights_config_t = NULL;
    int lights_is_active = 0;
    int resfile = 0;
    char *lights_config_json = NULL;
    cJSON *lights_status = NULL;
    char *new_test_var = NULL;
    lights_config_json = readFile("/spiffs/lights/config.json");
    lights_config_t = cJSON_Parse(lights_config_json);

    lights_status = cJSON_GetObjectItemCaseSensitive(lights_config_t, "status");

    if (cJSON_IsString(lights_status) && (lights_status->valuestring != NULL))
    {
        lights_config_json = lights_status->valuestring;
        lights_is_active = atoi(lights_config_json);
    }

    if (lights_is_active == 0)
    {
        lights_status->valuestring = "1";
        lights_config_json = cJSON_Print(lights_config_t);
        resfile = writeFile("/spiffs/lights/config.json", lights_config_json);
        ESP_LOGI(LIGHTS, "The lights are on");
    }
    else
    {
        lights_config_json = lights_status->valuestring = "0";
        lights_config_json = cJSON_Print(lights_config_t);
        ESP_LOGI(LIGHTS, "The lights are Off");
        resfile = writeFile("/spiffs/lights/config.json", lights_config_json);
    }
    if (resfile == 1)
    {
        lights_config_t = NULL;
        lights_config_json = NULL;
    }
    else
    {
        ESP_LOGE(LIGHTS, "Error to writeFile");
    }
}
