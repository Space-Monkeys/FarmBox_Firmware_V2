/* ESP HTTP Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_crt_bundle.h"
#include "tds_sensor.h"
#include "DHT22.h"
#include "ph_sensor.h"
#include "webserver.h"
#include "filesystem.h"
#include "water_pump.h"
#include "requests.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "HTTP_CLIENT";
static const char *TDS = "TDS_SENSOR";
static const char *DHT_TAG = "DHT_SENSOR";
static const char *PH_TAG = "PH_SENSOR";
static const char *REST_WEBSERVER = "REST_WEBSERVER";

#define DHT_22_GPIO 4
#define TDS_NUM_SAMPLES 3
#define TDS_SAMPLE_PERIOD 20

const char *API_FARMBOX_HOST = "spacemonkeys.com.br";

int API_FARMBOX_PORT = 8320;

float sampleDelay = (TDS_SAMPLE_PERIOD / TDS_NUM_SAMPLES) * 1000;

/* void tds_task(void *pvParameters)
{
    ESP_LOGI(TDS, "TDS Measurement Control Task: Starting");
    while (1)
    {
        ESP_LOGI(TDS, "TDS Measurement Control Task: Read TDS Sensor");
        enable_tds_sensor();
        float sensorReading = read_tds_sensor(TDS_NUM_SAMPLES, sampleDelay);
        float tdsResult = convert_to_ppm(sensorReading);
        //    printf("TDS Reading = %f ppm\n", tdsResult);
        ESP_LOGW(TDS, "TDS Reading = %f ppm\n", tdsResult);
        disable_tds_sensor();

        ESP_LOGI(TDS, "TDS Measurement Control Task: Sleeping 1 minute");
        vTaskDelay(((1000 / portTICK_PERIOD_MS) * 1) * 1); //delay in minutes between measurements
    }
} */
void DHT_task(void *pvParameter)
{
    setDHTgpio(DHT_22_GPIO);
    ESP_LOGI(TDS, "Climatic Measurement Control Task: DHT22 Sensor");
    const TickType_t xDelay = 60000 / portTICK_PERIOD_MS;
    while (1)
    {

        ESP_LOGI(DHT_TAG, "Climatic Measurement Control Task: Read DHT22 Sensor");
        int ret = readDHT();

        errorHandler(ret);

        ESP_LOGW(DHT_TAG, "Humidity: %.1f", getHumidity());
        ESP_LOGW(DHT_TAG, "Temperature: %.1f", getTemperature());

        const char *BODY_DATA = "{ \"id\": 666, \"value\": 0}";

        post_request(API_FARMBOX_HOST, "/api/v3/conductivity", BODY_DATA);

        ESP_LOGI(DHT_TAG, "Climatic Measurement Control Task: Sleeping 1 minute");
        vTaskDelay(xDelay); //delay in minutes between measurements
    }
}
void PH_Task(void *pvParameter)
{
    ESP_LOGI(PH_TAG, "Water Measurement Control Task: PH Sensor");
    config_ph_pins();
    setDHTgpio(4);
    while (1)
    {

        ESP_LOGI(PH_TAG, "Water Measurement Control Task: Reading PH Sensor....");
        float sensorReading = read_ph_sensor(25, 20);

        ESP_LOGW(PH_TAG, "PH = %f", sensorReading);

        int ret = readDHT();

        errorHandler(ret);

        ESP_LOGW(DHT_TAG, "Humidity: %.1f", getHumidity());
        ESP_LOGW(DHT_TAG, "Temperature: %.1f", getTemperature());

        float tds_sensor = read_tds_sensor(TDS_NUM_SAMPLES, sampleDelay);
        float tdsResult = convert_to_ppm(tds_sensor);
        //    printf("TDS Reading = %f ppm\n", tdsResult);
        ESP_LOGW(TDS, "TDS Reading = %f ppm\n", tdsResult);

        ESP_LOGI(PH_TAG, "Water Measurement Control Task: Sleeping 1 minute");
        vTaskDelay(((1000 / portTICK_PERIOD_MS) * 1) * 1); //delay in minutes between measurements
    }
}

void pump_task(void *pvParameter)
{
    load_pump_configuration();
}

void app_main(void)
{
    //################################ DEFAULTS #################################

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    vTaskDelay(1000 / portTICK_RATE_MS);

    ESP_ERROR_CHECK(example_connect());

    //################################ FILESYSTEM ################################
    filesystem_init();

    //################################ WEBSERVER #################################

    start_webserver();

    //################################ TASKS #################################

    //xTaskCreate(&pump_task, "pump_task", 2048, NULL, 5, NULL);
    //xTaskCreate(&tds_task, "tds_task", 2048, NULL, 5, NULL);
    xTaskCreate(&DHT_task, "DHT_task", 4096, NULL, 5, NULL);
    //xTaskCreate(&PH_Task, "PH_Task", 2048, NULL, 5, NULL);
    //xTaskCreate(&web_server, "web_server", 2048, NULL, 5, NULL);
}
