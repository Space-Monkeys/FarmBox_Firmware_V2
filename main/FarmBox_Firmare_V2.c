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
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "tds_sensor.h"
#include "DHT22.h"
#include "esp_http_client.h"
#include "ph_sensor.h"
#include "webserver.h"
#include "filesystem.h"
#include "water_pump.h"

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

float sampleDelay = (TDS_SAMPLE_PERIOD / TDS_NUM_SAMPLES) * 1000;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // If user_data buffer is configured, copy the response into the buffer
            if (evt->user_data)
            {
                memcpy(evt->user_data + output_len, evt->data, evt->data_len);
            }
            else
            {
                if (output_buffer == NULL)
                {
                    output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
                    output_len = 0;
                    if (output_buffer == NULL)
                    {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                memcpy(output_buffer + output_len, evt->data, evt->data_len);
            }
            output_len += evt->data_len;
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            if (output_buffer != NULL)
            {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        break;
    }
    return ESP_OK;
}

void tds_task(void *pvParameters)
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
        /* 
        //Send Request

        esp_http_client_config_t config = {
            .host = "spacemonkeys.com.br",
            .path = "/",
            .transport_type = HTTP_TRANSPORT_OVER_TCP,
            .event_handler = _http_event_handler,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);

        // POST

        const char *post_data = "id=1&value=500";
        esp_http_client_set_url(client, "/api/v3/conductivity");
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_post_field(client, post_data, strlen(post_data));

        esp_err_t err = esp_http_client_perform(client); //Is necessary?
        err = esp_http_client_perform(client);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                     esp_http_client_get_status_code(client),
                     esp_http_client_get_content_length(client));
        }
        else
        {
            ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client); */

        ESP_LOGI(TDS, "TDS Measurement Control Task: Sleeping 1 minute");
        vTaskDelay(((1000 / portTICK_PERIOD_MS) * 1) * 1); //delay in minutes between measurements
    }
}
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

        // -- wait at least 2 sec before reading again ------------
        // The interval of whole process must be beyond 2 seconds !!

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
    xTaskCreate(&DHT_task, "DHT_task", 2048, NULL, 5, NULL);
    //xTaskCreate(&PH_Task, "PH_Task", 2048, NULL, 5, NULL);
    //xTaskCreate(&web_server, "web_server", 2048, NULL, 5, NULL);
}
