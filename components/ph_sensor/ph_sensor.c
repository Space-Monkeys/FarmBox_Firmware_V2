/* TDS Sensor Example

   Mark Benson 2018

   Licence: MIT (or whatever is used by the rest of this project)
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"

#define PH_ANALOG_GPIO ADC1_CHANNEL_6 //ADC1 is availalbe on pins 15, 34, 35 & 36
static const char *PH_SENSOR = "PH SENSOR INFO";

void config_ph_pins()
{
    ESP_LOGI(PH_SENSOR, "Configure pins required for PH sensor.");
    // Pin to read the TDS sensor analog output
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(PH_ANALOG_GPIO, ADC_ATTEN_DB_11);
}

void ph_calibration(int numSamples, float samplePeriod)
{
    float sampleDelay = (samplePeriod / numSamples) * 1000;
    uint32_t runningSampleValue = 0;
    ESP_LOGI(PH_SENSOR, "================== PH Calibration Mode ================== ");
    for (int i = 0; i < numSamples; i++)
    {
        int analogSample = adc1_get_raw(PH_ANALOG_GPIO);
        ESP_LOGI(PH_SENSOR, "Read analog value %d then sleep for %f milli seconds.", analogSample, sampleDelay);
        runningSampleValue = runningSampleValue + analogSample;
        vTaskDelay(sampleDelay / portTICK_PERIOD_MS);
    }

    float tdsAverage = (runningSampleValue / numSamples) * (3.3 / 4095);
    ESP_LOGI(PH_SENSOR, "Calculated average = %f", tdsAverage);
}

float read_ph_sensor(int numSamples, float samplePeriod)
{

    float sampleDelay = (samplePeriod / numSamples) * 1000;
    // Take n sensor readings every p millseconds where n is numSamples, and p is sampleDelay.
    // Return the average sample value.
    uint32_t runningSampleValue = 0;

    for (int i = 0; i < numSamples; i++)
    {
        // Read analogue value
        int analogSample = adc1_get_raw(PH_ANALOG_GPIO);
        //ESP_LOGI(PH_SENSOR, "Read analog value %d then sleep for %f milli seconds.", analogSample, sampleDelay);
        runningSampleValue = runningSampleValue + analogSample;
        vTaskDelay(sampleDelay / portTICK_PERIOD_MS);
    }

    float tdsAverage = (runningSampleValue / numSamples) * (3.3 / 4095); //convert the analog into millivolt
    float phValue = -5.70 * tdsAverage + (21.34 - 0.5);                  // I understood nothing
    ESP_LOGI(PH_SENSOR, "Calculated average = %f", tdsAverage);
    return phValue;
}
