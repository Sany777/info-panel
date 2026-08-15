#include "adc_reader.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include <stdio.h>

#define ADC_CHANNEL ADC_CHANNEL_4 // GPIO32 (ESP32)
#define ADC_ATTEN ADC_ATTEN_DB_0
#define ADC_MAX_VALUE 4095 // 12-bit
#define VOLT_DIV_CONST 4.15F
#define VREF 1100.0F

static const char *TAG = "ADC_READER";
static adc_oneshot_unit_handle_t adc1_handle;

void
adc_reader_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));
    ESP_LOGI(TAG, "ADC Initialized");
}

float
device_get_voltage(void)
{
    int adc_value = 0;
    int sum       = 0;

    for (int i = 0; i < MESUR_NUM; i++) {
        adc_oneshot_read(adc1_handle, ADC_CHANNEL, &adc_value);
        sum += adc_value;
    }
    adc_value = sum / MESUR_NUM;

    return ((float)adc_value * (VREF / 1000.0f) * VOLT_DIV_CONST) / ADC_MAX_VALUE;
}
