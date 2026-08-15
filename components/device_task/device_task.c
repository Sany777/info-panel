#include "device_task.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "portmacro.h"
#include "stdbool.h"
#include "string.h"

#include "adc_reader.h"
#include "clock_module.h"
#include "device_common.h"
#include "display_icons.h"
#include "epaper_adapter.h"
#include "forecast_http_client.h"
#include "sdkconfig.h"
#include "setting_server.h"
#include "sound_generator.h"
#include "toolbox.h"
#include "wifi_service.h"

enum TimeoutMS {
    TIMEOUT_SEC           = 1000,
    TIMEOUT_10_SEC        = 10 * TIMEOUT_SEC,
    TIMEOUT_UPDATE_SCREEN = 19 * TIMEOUT_SEC,
    TIMEOUT_MINUTE        = 60 * TIMEOUT_SEC,
    TIMEOUT_FOUR_MINUTE   = 4 * TIMEOUT_MINUTE,
    TIMEOUT_HOUR          = 60 * TIMEOUT_MINUTE,
    TIMEOUT_4_HOUR        = 4 * TIMEOUT_HOUR,
    DELAY_UPDATE_FORECAST = 32 * TIMEOUT_MINUTE,
    LONG_PRESS_TIME       = TIMEOUT_SEC,
};

enum TaskDelay {
    DELAY_SERV = 100,
};

static int delay_update_forecast = 2 * TIMEOUT_MINUTE;
static esp_timer_handle_t touch_timer;

static void
check_battery_voltage()
{
    float volt_val = device_get_voltage();

    if (volt_val < 3.2) {
        device_set_pin(PIN_EP_EN, 0);
        start_signale_series(100, 20);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        esp_deep_sleep(UINT64_MAX);
    } else if (volt_val < 3.5) {
        start_signale_series(50, 20);
    }
}

static void
draw_status_bar(float voltage)
{
    epaper_display_image(265, 111, 24, 24, voltage < 3.5 ? RED : BLACK,
                         get_battery_icon_bitmap(battery_voltage_to_percentage(voltage)));
}

static void
draw_forecast_main(int data_indx, int cur_hour, bool is_day)
{
    epaper_printf(50, 118, FONT_SIZE_12, BLACK, "Sunrise:%d:%2.2d Sunset:%d:%2.2d", service_data.sunrise_hour,
                  service_data.sunrise_min, service_data.sunset_hour, service_data.sunset_min);
    epaper_display_image(230, -7, 64, 64, BLACK, get_forecast_data_icon(service_data.id_list[data_indx], is_day));
    epaper_printf(100, 17, FONT_SIZE_20, BLACK, "%+d*C", service_data.temp_list[data_indx]);
    epaper_printf_centered(48, FONT_SIZE_20, BLACK, "%s", service_data.desciption[data_indx]);
}

static void
draw_forecast_timeline(int udt)
{
    for (int i = 0; i < FORECAST_LIST_SIZE; ++i) {
        if (udt > 23)
            udt %= 24;
        int rect_x0 = 13 + i * 46;
        int rect_x1 = rect_x0 + 40;
        draw_rect(rect_x0, 68, rect_x1, 112, RED, false);
        epaper_printf(16 + i * 46, 70, FONT_SIZE_12, BLACK, "%d:00", udt);
        epaper_printf(22 + i * 46, 85, FONT_SIZE_12, BLACK, "%+d", service_data.temp_list[i]);
        epaper_printf(23 + i * 46, 100, FONT_SIZE_12, BLACK, "%d%%", service_data.pop_list[i]);
        udt += 3;
    }
}

static void
show_screen()
{
    int cur_hour      = get_cur_time_tm()->tm_hour;
    const bool is_day = cur_hour <= service_data.sunset_hour && cur_hour > service_data.sunrise_hour;
    float voltage     = device_get_voltage();

    draw_status_bar(voltage);

    if (main_data.city_name[0] != '\0') {
        epaper_printf(5, 5, FONT_SIZE_16, BLACK, "%s", main_data.city_name);
    }

    unsigned bits = device_get_state();
    if (service_data.update_data_time == NO_DATA && !(bits & BIT_FORECAST_OK)) {
        if (bits & BIT_STA_CONF_OK) {
            epaper_print_centered_str(80, FONT_SIZE_16, BLACK, "Updating data");
        } else if (bits & BIT_ERR_SSID_NOT_FOUND) {
            epaper_print_centered_str(80, FONT_SIZE_16, BLACK, "No wifi network found");
        } else {
            epaper_print_centered_str(80, FONT_SIZE_16, BLACK, "No data available");
        }
    } else {
        int udt       = service_data.update_data_time;
        int data_indx = get_actual_forecast_data_index(cur_hour, udt);
        if (data_indx == NO_DATA) {
            epaper_printf_centered(60, FONT_SIZE_16, BLACK, "Data update time %d:%02d", udt,
                                   service_data.update_data_min);
        } else {
            epaper_printf(5, 25, FONT_SIZE_12, RED, "%d:%02d", udt, service_data.update_data_min);
            epaper_printf(5, 40, FONT_SIZE_12, RED, "%02d.%02d", service_data.update_data_day,
                          service_data.update_data_mon);
            draw_forecast_main(data_indx, cur_hour, is_day);
            draw_forecast_timeline(udt);
        }
    }
}

static void
touch_poll_timer_cb(void *arg)
{
    static int current_but_id         = NO_DATA;
    static long long press_start_time = 0;
    static bool long_press_handled    = false;

    if (device_get_state() & BIT_DEVICE_BUSY) {
        return;
    }

    int but_val = device_get_touch_but_state();

    if (but_val != NO_DATA) {
        if (current_but_id != but_val) {
            current_but_id     = but_val;
            press_start_time   = esp_timer_get_time();
            long_press_handled = false;
        } else {
            long long hold_time = (esp_timer_get_time() - press_start_time);
            if (!long_press_handled && hold_time > LONG_PRESS_TIME) {
                long_press_handled = true;
                short_signale();

                if (but_val == TOUCH_BUT_LEFT) {
                    device_set_state(BIT_START_SERVER);
                } else {
                    device_set_state(BIT_UPDATE_FORECAST_DATA);
                }
            }
        }
    } else {
        current_but_id = NO_DATA;
    }
}

static void
handle_wakeup()
{
    device_set_pin(PIN_EP_EN, 1);
    device_set_state(BIT_DEVICE_BUSY);

    epaper_init();
    check_battery_voltage();

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    ESP_LOGI("DEVICE_TASK", "Waking up, cause: %d", cause);

    if (cause == ESP_SLEEP_WAKEUP_TIMER || cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
        device_set_state(BIT_UPDATE_FORECAST_DATA);
    } else {
        short_signale();
        if (cause == ESP_SLEEP_WAKEUP_TOUCHPAD) {
            int touch_pad = device_get_touch_but_state();
            for (int i = 0; i < 20 && touch_pad == NO_DATA; i++) {
                vTaskDelay(10 / portTICK_PERIOD_MS);
                touch_pad = device_get_touch_but_state();
            }
            ESP_LOGI("DEVICE_TASK", "Touch pad woken up, pad: %d", touch_pad);
            if (touch_pad == TOUCH_BUT_LEFT) {
                device_set_state(BIT_START_SERVER);
            } else {
                device_set_state(BIT_UPDATE_FORECAST_DATA);
            }
        } else {
            device_set_state(BIT_UPDATE_FORECAST_DATA);
        }
    }
}

static void
fetch_forecast()
{
    ESP_LOGI("DEVICE_TASK", "Updating forecast data");
    int esp_res;
    struct tm *tinfo;
    static bool fail_init_sntp = false;

    esp_res = connect_sta(main_data.ssid, main_data.pwd);
    if (esp_res == ESP_OK) {
        device_set_state(BIT_STA_CONF_OK);
        if (!(device_get_state() & BIT_IS_TIME)) {
            init_sntp();
            device_wait_bits(BIT_IS_TIME);
            stop_sntp();
        }
        esp_res = update_forecast_data(main_data.city_name, main_data.api_key);
    }

    if (esp_res == ESP_OK) {
        tinfo = get_cur_time_tm();
        if (fail_init_sntp || service_data.update_data_time > tinfo->tm_hour) {
            esp_restart();
        }
        service_data.update_data_time = tinfo->tm_hour;
        service_data.update_data_min  = tinfo->tm_min;
        service_data.update_data_day  = tinfo->tm_mday;
        service_data.update_data_mon  = tinfo->tm_mon + 1;
        if (!(device_get_state() & BIT_FORECAST_OK)) {
            delay_update_forecast = DELAY_UPDATE_FORECAST;
            device_set_state(BIT_FORECAST_OK);
        }
    } else {
        device_clear_state(BIT_FORECAST_OK);
        if (!fail_init_sntp && service_data.update_data_time == NO_DATA) {
            fail_init_sntp = true;
        }
        if (delay_update_forecast < DELAY_UPDATE_FORECAST) {
            delay_update_forecast *= 2;
        }
    }
    wifi_stop();
    device_set_state(BIT_UPDATE_SCREEN);
}

static void
run_settings_server()
{
    ESP_LOGI("DEVICE_TASK", "Starting AP setting server");

    if (start_ap() != ESP_OK) {
        device_set_state(BIT_GOTO_SLEEP);
        return;
    }

    unsigned st_bits = device_wait_bits(BIT_IS_AP_CONNECTION);
    if (!(st_bits & BIT_IS_AP_CONNECTION) || init_server(network_buf) != ESP_OK) {
        wifi_stop();
        device_set_state(BIT_GOTO_SLEEP);
        return;
    }

    int wait_client_timeout = 0;
    bool open_sesion        = false;

    float voltage = device_get_voltage();
    epaper_clear();
    draw_status_bar(voltage);
    epaper_print_centered_str(15, FONT_SIZE_16, BLACK, "AP MODE ACTIVE");
    epaper_printf(10, 35, FONT_SIZE_12, BLACK, "SSID: %s", CONFIG_WIFI_AP_SSID);
    if (strlen(CONFIG_WIFI_AP_PASSWORD) > 0) {
        epaper_printf(10, 50, FONT_SIZE_12, BLACK, "PWD: %s", CONFIG_WIFI_AP_PASSWORD);
    } else {
        epaper_printf(10, 50, FONT_SIZE_12, BLACK, "PWD: <OPEN>");
    }
    epaper_printf(10, 65, FONT_SIZE_12, BLACK, "IP: 192.168.4.1");
    epaper_printf(10, 80, FONT_SIZE_12, BLACK, "City: %s",
                  main_data.city_name[0] != '\0' ? main_data.city_name : "Not set");
    epaper_printf(10, 95, FONT_SIZE_12, BLACK, "Token: %s", main_data.api_key[0] != '\0' ? "OK" : "Missing");
    epaper_printf(10, 110, FONT_SIZE_12, BLACK, "Bat: %.2fV", voltage);
    epaper_update();

    while (device_get_touch_but_state() != NO_DATA) {
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    while (1) {
        st_bits = device_get_state();
        if (open_sesion) {
            if (!(st_bits & BIT_IS_AP_CLIENT)) {
                break;
            }
        } else if (st_bits & BIT_IS_AP_CLIENT) {
            wait_client_timeout = 0;
            open_sesion         = true;
        } else if (wait_client_timeout > 2 * TIMEOUT_MINUTE) {
            break;
        } else {
            wait_client_timeout += DELAY_SERV;
        }

        bool button_touched = false;
        int delay_steps     = DELAY_SERV / 50;
        for (int i = 0; i < delay_steps; i++) {
            if (device_get_touch_but_state() != NO_DATA) {
                button_touched = true;
                break;
            }
            EventBits_t t_bits = device_wait_bits_clear(BIT_START_SERVER, 50 / portTICK_PERIOD_MS);
            if (t_bits & BIT_START_SERVER) {
                button_touched = true;
                break;
            }
        }

        if (button_touched) {
            short_signale();
            while (device_get_touch_but_state() != NO_DATA) {
                vTaskDelay(50 / portTICK_PERIOD_MS);
            }
            break;
        }
    }

    deinit_server();
    bool change_settings = device_commit_changes();
    wifi_stop();

    if (change_settings) {
        device_clear_state(BIT_FORECAST_OK);
        device_set_state(BIT_UPDATE_FORECAST_DATA);
    } else {
        device_set_state(BIT_GOTO_SLEEP);
    }
}

static void
render_and_sleep()
{
    ESP_LOGI("DEVICE_TASK", "Updating screen");
    epaper_clear();
    show_screen();
    epaper_update();
    device_set_state(BIT_GOTO_SLEEP);
}

static void
goto_sleep()
{
    ESP_LOGI("DEVICE_TASK", "Going to sleep");
    device_wait_bits_to_clear(BIT_WAIT_SIGNALE);

    vTaskDelay(100 / portTICK_PERIOD_MS);

    device_set_pin(PIN_EP_EN, 0);
    esp_sleep_enable_timer_wakeup(delay_update_forecast * 1000);

    device_clear_state(BIT_DEVICE_BUSY);
    esp_light_sleep_start();

    device_set_state(BIT_WAKEUP);
}

static void
main_task(void *pvParameters)
{
    while (1) {
        EventBits_t bits = device_wait_bits_clear(BIT_WAKEUP | BIT_UPDATE_FORECAST_DATA | BIT_UPDATE_SCREEN |
                                                      BIT_START_SERVER | BIT_GOTO_SLEEP,
                                                  portMAX_DELAY);

        if (bits & BIT_WAKEUP) {
            handle_wakeup();
        }
        if (bits & BIT_UPDATE_FORECAST_DATA) {
            fetch_forecast();
        }
        if (bits & BIT_UPDATE_SCREEN) {
            render_and_sleep();
        }
        if (bits & BIT_START_SERVER) {
            run_settings_server();
        }
        if (bits & BIT_GOTO_SLEEP) {
            goto_sleep();
        }
    }
}

int
task_init()
{
    set_offset(main_data.time_offset, main_data.dst_enabled);

    esp_timer_create_args_t timer_args = {.callback = &touch_poll_timer_cb, .name = "touch_poll"};
    esp_timer_create(&timer_args, &touch_timer);
    esp_timer_start_periodic(touch_timer, 100000);

    xTaskCreate(main_task, "main_task", 20000, NULL, 5, NULL);

    device_set_state(BIT_WAKEUP);

    return ESP_OK;
}
