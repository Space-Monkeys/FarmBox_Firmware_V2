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
#include "cJSON.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_crt_bundle.h"
#include "tds_sensor.h"
#include "lights.h"
#include "DHT22.h"
#include "ph_sensor.h"
#include "webserver.h"
#include "filesystem.h"
#include "water_pump.h"
#include "requests.h"
#include "time.h"
#include "esp_sntp.h"
#include "driver/gpio.h"
#include "cron.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "HTTP_CLIENT";
static const char *TDS = "TDS_SENSOR";
static const char *DHT_TAG = "DHT_SENSOR";
static const char *PH_TAG = "PH_SENSOR";
static const char *REST_WEBSERVER = "REST_WEBSERVER";
static const char *CLOCK_TAG = "CLOCK_TASK";

#define TDS_NUM_SAMPLES 3
#define GPIO_INPUT 16
#define GPIO_OUTPUT 18
#define TDS_SAMPLE_PERIOD 20

const char *API_FARMBOX_HOST = "spacemonkeys.com.br";

int API_FARMBOX_PORT = 8320;
int DHT_22_GPIO = 4;

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
void print_time(time_t t)
{
    char strftime_buf[64];
    struct tm timeinfo;
    localtime_r(&t, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The date/time in Paris is: %s", strftime_buf);
}
void DHT_task(void *pvParameter)
{
    setDHTgpio(DHT_22_GPIO);
    ESP_LOGI(DHT_TAG, "Climatic Measurement Control Task: Read DHT22 Sensor");
    int ret = readDHT();
    errorHandler(ret); //TODO: #2 Add validation in the future when this is going to an api, Recursive function doesn't work, for some reason I have multiple returns and so the function is called multiple times ESP_LOGW(DHT_TAG, "Humidity: %.1f", getHumidity());
    ESP_LOGW(DHT_TAG, "Humidity: %.1f", getHumidity());
    ESP_LOGW(DHT_TAG, "Temperature: %.1f", getTemperature());
    /* 
        const char *BODY_DATA = "{ \"id\": 666, \"value\": 0}";
        post_request(API_FARMBOX_HOST, "/api/v3/conductivity", BODY_DATA); 
    */
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

void check_time()
{
    int status = sntp_get_sync_status();
    ESP_LOGI(TAG, "SNTP sync status: %i", status);
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Brazil is: %s", strftime_buf);
}

void manage_pump(cron_job *job)
{
    ESP_LOGI(TAG, "Disabling pump...");
    ESP_LOGI(TAG, "Next execution is: ");
    print_time(job->next_execution);
    return;
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
    vTaskDelay(2000 / portTICK_RATE_MS);

    //################################ Time #################################

    ESP_LOGI(TAG, "Setting clock...");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Setting time zone...");
    setenv("TZ", "<-03>3", 1);
    tzset();
    check_time();

    //################################ FILESYSTEM ################################
    filesystem_init();

    //################################ WEBSERVER #################################

    start_webserver();
    vTaskDelay(2000 / portTICK_RATE_MS);

    //################################ CRON #################################

    // jobs[1] = cron_job_create("0 0 8 * * *", manage_switcher, (void *)0);

    //################################ TASKS #####################################
    cJSON *json_task = NULL;
    cJSON *json_value = NULL;
    const char *value_char = NULL;
    char *taks = NULL;
    cron_job *jobs[2];

    taks = readFile("/spiffs/tasks/main.json");
    ESP_LOGI(TAG, "%s", taks);
    json_task = cJSON_Parse(taks);

    if (json_task == NULL)
    {
        ESP_LOGE(TAG, "Não foi possivel montar o json");
        return;
    }
    json_value = cJSON_GetObjectItemCaseSensitive(json_task, "pump");
    if (json_value != NULL && cJSON_IsString(json_value))
    {
        value_char = json_value->valuestring;
        ESP_LOGI(TAG, "Value for pump task: %s", value_char);
        ESP_LOGI(TAG, "Setting cron job for pump task...");
        jobs[0] = cron_job_create(value_char, pump_actions, (void *)0); //TODO: #3 Remember to limit when setting the time the pump is on to never be greater than the task time
        value_char = NULL;
        json_value = NULL;
    }

    json_value = cJSON_GetObjectItemCaseSensitive(json_task, "ligths");
    if (json_value != NULL && cJSON_IsString(json_value))
    {
        value_char = json_value->valuestring;
        ESP_LOGI(TAG, "Value for ligths task: %s", value_char);
        ESP_LOGI(TAG, "Setting cron job for ligths task...");
        jobs[1] = cron_job_create(value_char, light_actions, (void *)0); //TODO: Remember to limit when setting the time the pump is on to never be greater than the task time
        value_char = NULL;
        json_value = NULL;
    }
    json_value = cJSON_GetObjectItemCaseSensitive(json_task, "dht22_sensor");
    if (json_value != NULL && cJSON_IsString(json_value))
    {
        value_char = json_value->valuestring;
        ESP_LOGI(TAG, "Value for dht22 task: %s", value_char);
        ESP_LOGI(TAG, "Setting cron job for dht22 task...");
        jobs[2] = cron_job_create(value_char, DHT_task, (void *)0); //TODO: Remember to limit when setting the time the pump is on to never be greater than the task time
        value_char = NULL;
        json_value = NULL;
    }

    ESP_LOGI(TAG, "Starting cron job...");
    cron_start();

    //################################ TASKS #################################
    //xTaskCreate(&pump_task, "pump_task", 2048, NULL, 5, NULL);
    //xTaskCreate(&tds_task, "tds_task", 2048, NULL, 5, NULL);
    // xTaskCreate(&DHT_task, "DHT_task", 4096, NULL, 5, NULL);
    //xTaskCreate(&PH_Task, "PH_Task", 2048, NULL, 5, NULL);
    //xTaskCreate(&task_manager, "task_manager", 10096, NULL, 5, NULL);
}
