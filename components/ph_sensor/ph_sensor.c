#include <stdio.h>
#include "ph_sensor.h"
#include <driver/adc.h>

static const char *PH_I = "PH_INTERNAL";

void calibration(void)
{
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_0db);
    int val = adc1_get_voltage(ADC1_CHANNEL_0);
    ESP_LOGI(PH_I, val);
}
