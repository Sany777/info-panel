#include "device_common.h"
#include "sound_generator.h"

#include "device_macro.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "portmacro.h"

#include "driver/gpio.h"
#include "driver/touch_sens.h"
#include "esp_sleep.h"
#include "esp_timer.h"

static touch_sensor_handle_t sens_handle        = NULL;
static touch_channel_handle_t right_chan_handle = NULL;
static touch_channel_handle_t left_chan_handle  = NULL;

int
device_get_touch_but_state()
{
    uint32_t touch_val;

    if (right_chan_handle) {
        if (touch_channel_read_data(right_chan_handle, TOUCH_CHAN_DATA_TYPE_SMOOTH, &touch_val) == ESP_OK) {
            if (touch_val < TOUCH_THRESHOLD) {
                return TOUCH_BUT_RIGHT;
            }
        }
    }

    if (left_chan_handle) {
        if (touch_channel_read_data(left_chan_handle, TOUCH_CHAN_DATA_TYPE_SMOOTH, &touch_val) == ESP_OK) {
            if (touch_val < TOUCH_THRESHOLD) {
                return TOUCH_BUT_LEFT;
            }
        }
    }

    return NO_DATA;
}

void
device_gpio_init()
{
    device_set_pin(PIN_BUZZER, 0);
    device_set_pin(PIN_EP_EN, 0);
    gpio_set_direction((gpio_num_t)PIN_TOUCH_RIGHT, GPIO_MODE_INPUT);
    gpio_set_level((gpio_num_t)PIN_TOUCH_RIGHT, 0);

    touch_sensor_sample_config_t sample_cfg[1] = {
        TOUCH_SENSOR_V1_DEFAULT_SAMPLE_CONFIG(5.0, TOUCH_VOLT_LIM_L_0V5, TOUCH_VOLT_LIM_H_1V7)};
    touch_sensor_config_t sens_cfg = TOUCH_SENSOR_DEFAULT_BASIC_CONFIG(1, sample_cfg);
    ESP_ERROR_CHECK(touch_sensor_new_controller(&sens_cfg, &sens_handle));

    touch_channel_config_t chan_cfg = {
        .abs_active_thresh = {TOUCH_THRESHOLD},
        .charge_speed      = TOUCH_CHARGE_SPEED_7,
        .init_charge_volt  = TOUCH_INIT_CHARGE_VOLT_DEFAULT,
        .group             = TOUCH_CHAN_TRIG_GROUP_BOTH,
    };

    ESP_ERROR_CHECK(touch_sensor_new_channel(sens_handle, TOUCH_BUT_LEFT, &chan_cfg, &left_chan_handle));
    ESP_ERROR_CHECK(touch_sensor_new_channel(sens_handle, TOUCH_BUT_RIGHT, &chan_cfg, &right_chan_handle));

    touch_sensor_filter_config_t filter_cfg = TOUCH_SENSOR_DEFAULT_FILTER_CONFIG();
    ESP_ERROR_CHECK(touch_sensor_config_filter(sens_handle, &filter_cfg));

    touch_sleep_config_t slp_cfg = TOUCH_SENSOR_DEFAULT_LSLP_CONFIG();
    ESP_ERROR_CHECK(touch_sensor_config_sleep_wakeup(sens_handle, &slp_cfg));

    ESP_ERROR_CHECK(touch_sensor_enable(sens_handle));
    ESP_ERROR_CHECK(touch_sensor_start_continuous_scanning(sens_handle));
    ESP_ERROR_CHECK(esp_sleep_enable_touchpad_wakeup());
}

int IRAM_ATTR
device_set_pin(int pin, unsigned state)
{
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT_OUTPUT);
    return gpio_set_level((gpio_num_t)pin, state);
}
