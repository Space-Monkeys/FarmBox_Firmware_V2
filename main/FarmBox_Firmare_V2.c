#include <stdio.h>
#include <esp_log.h>
#include "tds_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

static const char *TAG = "FARMBOX_FIRMWARE";

#define TDS_NUM_SAMPLES 10
#define TDS_SAMPLE_PERIOD 20

float sampleDelay = (TDS_SAMPLE_PERIOD / TDS_NUM_SAMPLES) * 1000;

void tds_task(void *pvParameters)
{
    ESP_LOGI(TAG, "***********************************************************");
    ESP_LOGI(TAG, "TDS Measurement Control Task: Starting");
    while (1)
    {
        ESP_LOGI(TAG, "TDS Measurement Control Task: Read Sensor");
        enable_tds_sensor();
        float sensorReading = read_tds_sensor(TDS_NUM_SAMPLES, sampleDelay);
        float tdsResult = convert_to_ppm(sensorReading);
        printf("TDS Reading = %f ppm\n", tdsResult);
        disable_tds_sensor();
        ESP_LOGI(TAG, "TDS Measurement Control Task: Sleeping 1 minute");
        ESP_LOGI(TAG, "***********************************************************");
        vTaskDelay(((1000 / portTICK_PERIOD_MS) * 1) * 1); //delay in minutes between measurements
    }
}

void app_main()
{
    // Enable sensor then wait 10 seconds for reading to stabilise before reading
    // Read the sensor a few times and get an average
    // Convert reading to a Total Disolved Solids (TDS) value
    //expose_vref();
    config_pins();
    // Start TDS Task
    xTaskCreate(tds_task, "tds_task", 2048, NULL, 5, NULL);
}
