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
#include "time.h"
#include "esp_sntp.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "HTTP_CLIENT";
static const char *TDS = "TDS_SENSOR";
static const char *DHT_TAG = "DHT_SENSOR";
static const char *PH_TAG = "PH_SENSOR";
static const char *REST_WEBSERVER = "REST_WEBSERVER";
static const char *CLOCK_TAG = "CLOCK_TASK";

#define DHT_22_GPIO 4
#define TDS_NUM_SAMPLES 3
#define TDS_SAMPLE_PERIOD 20
TaskHandle_t Clock_TaskHandle;

const char *API_FARMBOX_HOST = "spacemonkeys.com.br";

int API_FARMBOX_PORT = 8320;

static void obtain_time(void);
static void initialize_sntp(void);

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
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

void scheduler_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Task Clock Init");
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900))
    {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }

    char strftime_buf[64];

    // Set timezone to Eastern Standard Time and print local time
    setenv("TZ", "<-03>3", 1);
    tzset();
    long int water_pump_interval = 15;
    long int next_pump_enable_time = now + water_pump_interval;

    while (1)
    {
        time(&now);

        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

        if (now >= next_pump_enable_time)
        {
            ESP_LOGW(CLOCK_TAG, "%s", "Bomba Ligada");
            next_pump_enable_time = now + water_pump_interval;
            ESP_LOGW(CLOCK_TAG, "Next enable pump time is in %ld", next_pump_enable_time);
        }

        ESP_LOGI(CLOCK_TAG, "The current date/time in São Paulo is: %s", strftime_buf);

        vTaskDelay(((1000 / portTICK_PERIOD_MS) * 1) * 1); //delay in minutes between measurements
    }
}

void task_manager(void *pvParameter)
{
    xTaskCreate(&scheduler_task, "scheduler_task", 2048, NULL, 5, &Clock_TaskHandle);
    int control = 1;
    while (1)
    {
        if (control == 1)
        {
            vTaskDelay(9000 / portTICK_PERIOD_MS);
            vTaskDelete(Clock_TaskHandle);
            ESP_LOGW(CLOCK_TAG, "%s", "Task Deleted");
            control = 0;
        }
        else
        {
            xTaskCreate(&scheduler_task, "scheduler_task", 2048, NULL, 5, &Clock_TaskHandle);
            control = 1;
        }
    }
}

static void obtain_time(void)
{
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}
static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
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

    //################################ FILESYSTEM ################################
    filesystem_init();

    //################################ WEBSERVER #################################

    start_webserver();

    //################################ TASKS #################################
    //xTaskCreate(&pump_task, "pump_task", 2048, NULL, 5, NULL);
    //xTaskCreate(&tds_task, "tds_task", 2048, NULL, 5, NULL);
    // xTaskCreate(&DHT_task, "DHT_task", 4096, NULL, 5, NULL);
    //xTaskCreate(&PH_Task, "PH_Task", 2048, NULL, 5, NULL);
    xTaskCreate(&task_manager, "task_manager", 2048, NULL, 5, NULL);
}
