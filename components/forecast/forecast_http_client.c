#include "forecast_http_client.h"

#include "cJSON.h"
#include "clock_module.h"
#include "device_common.h"
#include "device_macro.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"

#define SIZE_URL_BUF 250

char network_buf[NET_BUF_LEN];

static char url_buf[SIZE_URL_BUF];

static esp_err_t
http_event_handler(esp_http_client_event_t *evt)
{
    static int output_len = 0;
    switch (evt->event_id) {
    case HTTP_EVENT_ON_CONNECTED:
        output_len = 0;
        break;
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client)) {
            if (evt->user_data) {
                int copy_len = MIN(evt->data_len, (NET_BUF_LEN - 1 - output_len));
                if (copy_len > 0) {
                    memcpy((char *)evt->user_data + output_len, evt->data, copy_len);
                    output_len += copy_len;
                }
            }
        }
        break;
    case HTTP_EVENT_ON_FINISH:
    case HTTP_EVENT_DISCONNECTED:
        if (evt->user_data) {
            ((char *)evt->user_data)[output_len] = '\0';
        }
        output_len = 0;
        break;
    default:
        break;
    }
    return ESP_OK;
}

int
update_forecast_data(const char *city, const char *api_key)
{
    if (strnlen(city, MAX_STR_LEN) == 0 || strnlen(api_key, MAX_STR_LEN) != API_LEN) {
        return ESP_FAIL;
    }

    snprintf(url_buf, SIZE_URL_BUF,
             "https://api.openweathermap.org/data/2.5/forecast?q=%s&units=metric&cnt=%d&appid=%s", city,
             FORECAST_LIST_SIZE, api_key);

    esp_http_client_config_t config = {.url                         = url_buf,
                                       .event_handler               = http_event_handler,
                                       .user_data                   = (void *)network_buf,
                                       .method                      = HTTP_METHOD_GET,
                                       .buffer_size                 = NET_BUF_LEN,
                                       .auth_type                   = HTTP_AUTH_TYPE_NONE,
                                       .crt_bundle_attach           = esp_crt_bundle_attach,
                                       .skip_cert_common_name_check = true};

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err                   = esp_http_client_perform(client);

    int res = ESP_FAIL;
    if (err == ESP_OK) {
        cJSON *root = cJSON_Parse(network_buf);
        if (root) {
            cJSON *city_obj = cJSON_GetObjectItem(root, "city");
            if (city_obj) {
                cJSON *sunrise = cJSON_GetObjectItem(city_obj, "sunrise");
                if (sunrise) {
                    time_t time               = sunrise->valueint;
                    struct tm *tinfo          = localtime(&time);
                    service_data.sunrise_hour = tinfo->tm_hour;
                    service_data.sunrise_min  = tinfo->tm_min;
                }
                cJSON *sunset = cJSON_GetObjectItem(city_obj, "sunset");
                if (sunset) {
                    time_t time              = sunset->valueint;
                    struct tm *tinfo         = localtime(&time);
                    service_data.sunset_hour = tinfo->tm_hour;
                    service_data.sunset_min  = tinfo->tm_min;
                }
            }

            cJSON *list_arr = cJSON_GetObjectItem(root, "list");
            if (list_arr && cJSON_IsArray(list_arr)) {
                int count = MIN(cJSON_GetArraySize(list_arr), FORECAST_LIST_SIZE);
                memset(service_data.desciption, 0, sizeof(service_data.desciption));

                for (int i = 0; i < count; i++) {
                    cJSON *item = cJSON_GetArrayItem(list_arr, i);
                    if (!item)
                        continue;

                    cJSON *main_obj = cJSON_GetObjectItem(item, "main");
                    if (main_obj) {
                        cJSON *feels_like = cJSON_GetObjectItem(main_obj, "feels_like");
                        if (feels_like) {
                            service_data.temp_list[i] = feels_like->valuedouble;
                        }
                    }

                    cJSON *pop = cJSON_GetObjectItem(item, "pop");
                    if (pop) {
                        service_data.pop_list[i] = pop->valuedouble * 100;
                    }

                    cJSON *weather_arr = cJSON_GetObjectItem(item, "weather");
                    if (weather_arr && cJSON_IsArray(weather_arr)) {
                        cJSON *weather_item = cJSON_GetArrayItem(weather_arr, 0);
                        if (weather_item) {
                            cJSON *id = cJSON_GetObjectItem(weather_item, "id");
                            if (id) {
                                service_data.id_list[i] = id->valueint;
                            }
                            cJSON *desc = cJSON_GetObjectItem(weather_item, "description");
                            if (desc && desc->valuestring) {
                                strncpy(service_data.desciption[i], desc->valuestring,
                                        ARR_LEN(service_data.desciption[0]) - 1);
                            }
                        }
                    }
                }
                res = ESP_OK;
            }
            cJSON_Delete(root);
        }
    }

    esp_http_client_cleanup(client);
    return res;
}
