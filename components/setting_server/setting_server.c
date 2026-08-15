#include "setting_server.h"

#include "cJSON.h"
#include "stdbool.h"
#include <dirent.h>
#include <sys/stat.h>

#include "device_common.h"
#include "device_macro.h"
#include "esp_chip_info.h"
#include "esp_http_server.h"
#include "esp_mac.h"
#include "portmacro.h"
#include "string.h"
#include "toolbox.h"
#include "wifi_service.h"

static httpd_handle_t server;

static const char *MES_DATA_NOT_READ  = "Data not read";
static const char *MES_DATA_TOO_LONG  = "Data too long";
static const char *MES_NO_MEMORY      = "No memory";
static const char *MES_BAD_DATA_FOMAT = "wrong data format";
static const char *MES_SUCCESSFUL     = "Successful";

#define SEND_REQ_ERR(_req_, _str_)                                                                                     \
    do {                                                                                                               \
        httpd_resp_send_err((_req_), HTTPD_400_BAD_REQUEST, (_str_));                                                  \
        return ESP_FAIL;                                                                                               \
    } while (0)

#define SEND_SERVER_ERR(_req_, _str_)                                                                                  \
    do {                                                                                                               \
        httpd_resp_send_err((_req_), HTTPD_500_INTERNAL_SERVER_ERROR, (_str_));                                        \
        return ESP_FAIL;                                                                                               \
    } while (0)

void
server_stop()
{
    device_clear_state(BIT_SERVER_RUN);
}

static esp_err_t
index_redirect_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/index.html");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t
get_index_handler(httpd_req_t *req)
{
    extern const unsigned char index_html_start[] asm("_binary_index_html_start");
    extern const unsigned char index_html_end[] asm("_binary_index_html_end");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

static esp_err_t
get_css_handler(httpd_req_t *req)
{
    extern const unsigned char style_css_start[] asm("_binary_style_css_start");
    extern const unsigned char style_css_end[] asm("_binary_style_css_end");
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)style_css_start, style_css_end - style_css_start);
    return ESP_OK;
}

static esp_err_t
get_js_handler(httpd_req_t *req)
{
    extern const unsigned char script_js_start[] asm("_binary_script_js_start");
    extern const unsigned char script_js_end[] asm("_binary_script_js_end");
    httpd_resp_set_type(req, "text/javascript");
    httpd_resp_send(req, (const char *)script_js_start, script_js_end - script_js_start);
    return ESP_OK;
}

static esp_err_t
handler_close(httpd_req_t *req)
{
    httpd_resp_sendstr(req, "Goodbay!");
    vTaskDelay(500 / portTICK_PERIOD_MS);
    device_clear_state(BIT_SERVER_RUN);
    return ESP_OK;
}

static esp_err_t
handler_set_network(httpd_req_t *req)
{
    cJSON *root, *ssid_name_j, *pwd_wifi_j;
    int received;
    const size_t total_len = req->content_len;
    char *server_buf       = (char *)req->user_ctx;
    if (total_len >= NET_BUF_LEN) {
        SEND_REQ_ERR(req, MES_DATA_TOO_LONG);
    }
    received = httpd_req_recv(req, server_buf, total_len);
    if (received != total_len) {
        SEND_SERVER_ERR(req, MES_DATA_NOT_READ);
    }
    server_buf[received] = 0;
    root                 = cJSON_Parse(server_buf);
    if (!root) {
        SEND_SERVER_ERR(req, MES_NO_MEMORY);
    }

    ssid_name_j = cJSON_GetObjectItemCaseSensitive(root, "SSID");
    pwd_wifi_j  = cJSON_GetObjectItemCaseSensitive(root, "PWD");

    if (cJSON_IsString(ssid_name_j) && (ssid_name_j->valuestring != NULL)) {
        device_set_ssid(ssid_name_j->valuestring);
    }
    if (cJSON_IsString(pwd_wifi_j) && (pwd_wifi_j->valuestring != NULL)) {
        device_set_pwd(pwd_wifi_j->valuestring);
    }
    cJSON_Delete(root);
    httpd_resp_sendstr(req, MES_SUCCESSFUL);
    return ESP_OK;
}

static esp_err_t
handler_set_openweather_data(httpd_req_t *req)
{
    cJSON *root, *city_j, *key_j;
    int received;
    const int total_len = req->content_len;
    char *server_buf    = (char *)req->user_ctx;
    if (total_len >= NET_BUF_LEN) {
        SEND_REQ_ERR(req, MES_DATA_TOO_LONG);
    }
    received = httpd_req_recv(req, server_buf, total_len);
    if (received != total_len) {
        SEND_SERVER_ERR(req, MES_DATA_NOT_READ);
    }
    server_buf[received] = 0;
    root                 = cJSON_Parse(server_buf);
    if (!root) {
        SEND_SERVER_ERR(req, MES_NO_MEMORY);
    }
    city_j = cJSON_GetObjectItemCaseSensitive(root, "City");
    key_j  = cJSON_GetObjectItemCaseSensitive(root, "Key");
    if (cJSON_IsString(city_j) && (city_j->valuestring != NULL)) {
        device_set_city(city_j->valuestring);
    }
    if (cJSON_IsString(key_j) && (key_j->valuestring != NULL)) {
        device_set_key(key_j->valuestring);
    }
    cJSON_Delete(root);
    httpd_resp_sendstr(req, MES_SUCCESSFUL);
    return ESP_OK;
}

const char *
get_chip(int model_id)
{
    switch (model_id) {
    case 1:
        return "ESP32";
    case 2:
        return "ESP32-S2";
    case 3:
        return "ESP32-S3";
    case 5:
        return "ESP32-C3";
    case 6:
        return "ESP32-H2";
    case 12:
        return "ESP32-C2";
    default:
        break;
    }
    return "uknown";
}

static esp_err_t
handler_get_info(httpd_req_t *req)
{
    char *server_buf = (char *)req->user_ctx;
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    snprintf(server_buf, 200, "idf version %s\nchip %s\nrevision %u\nMAC %02X:%02X:%02X:%02X:%02X:%02X\nBuild %s", 
             IDF_VER, get_chip(chip_info.model),
             chip_info.revision,
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
             __DATE__);
    httpd_resp_sendstr(req, server_buf);
    return ESP_OK;
}

static esp_err_t
handler_give_data(httpd_req_t *req)
{
    char *data_to_send;
    cJSON *root;
    httpd_resp_set_type(req, "application/json");
    root = cJSON_CreateObject();
    if (!root) {
        SEND_REQ_ERR(req, MES_NO_MEMORY);
    }
    cJSON_AddStringToObject(root, "SSID", main_data.ssid);
    cJSON_AddStringToObject(root, "PWD", main_data.pwd);
    cJSON_AddStringToObject(root, "Key", main_data.api_key);
    cJSON_AddStringToObject(root, "City", main_data.city_name);
    cJSON_AddNumberToObject(root, "Hour", main_data.time_offset);
    cJSON_AddNumberToObject(root, "Dst", main_data.dst_enabled ? 1 : 0);
    data_to_send = cJSON_Print(root);

    if (!data_to_send) {
        cJSON_Delete(root);
        SEND_REQ_ERR(req, MES_NO_MEMORY);
    }
    httpd_resp_sendstr(req, data_to_send);
    free(data_to_send);
    data_to_send = NULL;
    cJSON_Delete(root);

    return ESP_OK;
}

static esp_err_t
set_offset_handler(httpd_req_t *req)
{
    cJSON *root, *hour_j, *dst_j;
    int received;
    const size_t total_len = req->content_len;
    char *server_buf       = (char *)req->user_ctx;
    if (total_len >= NET_BUF_LEN) {
        SEND_REQ_ERR(req, MES_DATA_TOO_LONG);
    }
    received = httpd_req_recv(req, server_buf, total_len);
    if (received != total_len) {
        SEND_SERVER_ERR(req, MES_DATA_NOT_READ);
    }
    server_buf[received] = 0;
    root                 = cJSON_Parse(server_buf);
    if (!root) {
        SEND_REQ_ERR(req, MES_BAD_DATA_FOMAT);
    }

    hour_j = cJSON_GetObjectItem(root, "Hour");
    dst_j  = cJSON_GetObjectItem(root, "Dst");

    if (hour_j) {
        int offset = 0;
        if (cJSON_IsNumber(hour_j)) {
            offset = hour_j->valueint;
        } else if (cJSON_IsString(hour_j)) {
            offset = atoi(hour_j->valuestring);
        }
        if (offset > 23 || offset < -23) {
            cJSON_Delete(root);
            SEND_REQ_ERR(req, MES_BAD_DATA_FOMAT);
        }
        device_set_offset(offset);
    }

    if (dst_j) {
        bool dst = false;
        if (cJSON_IsNumber(dst_j)) {
            dst = (dst_j->valueint != 0);
        } else if (cJSON_IsBool(dst_j)) {
            dst = cJSON_IsTrue(dst_j);
        }
        device_set_dst(dst);
    }

    cJSON_Delete(root);
    httpd_resp_sendstr(req, MES_SUCCESSFUL);
    return ESP_OK;
}

int
deinit_server()
{
    esp_err_t err = ESP_FAIL;
    if (server != NULL) {
        err = httpd_stop(server);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        server = NULL;
    }
    return err;
}

int
init_server(char *server_buf)
{
    if (server != NULL)
        return ESP_FAIL;
    httpd_config_t config   = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;
    config.uri_match_fn     = httpd_uri_match_wildcard;

    if (httpd_start(&server, &config) != ESP_OK) {
        return ESP_FAIL;
    }

    httpd_uri_t get_info = {.uri = "/info?", .method = HTTP_POST, .handler = handler_get_info, .user_ctx = server_buf};
    httpd_register_uri_handler(server, &get_info);

    httpd_uri_t get_setting = {
        .uri = "/data?", .method = HTTP_POST, .handler = handler_give_data, .user_ctx = server_buf};
    httpd_register_uri_handler(server, &get_setting);

    httpd_uri_t close_uri = {.uri = "/close", .method = HTTP_POST, .handler = handler_close, .user_ctx = server_buf};
    httpd_register_uri_handler(server, &close_uri);

    httpd_uri_t net_uri = {
        .uri = "/Network", .method = HTTP_POST, .handler = handler_set_network, .user_ctx = server_buf};
    httpd_register_uri_handler(server, &net_uri);

    httpd_uri_t api_uri = {
        .uri = "/Openweather", .method = HTTP_POST, .handler = handler_set_openweather_data, .user_ctx = server_buf};
    httpd_register_uri_handler(server, &api_uri);

    httpd_uri_t index_uri = {
        .uri = "/index.html", .method = HTTP_GET, .handler = get_index_handler, .user_ctx = server_buf};
    httpd_register_uri_handler(server, &index_uri);

    httpd_uri_t get_style = {
        .uri = "/style.css", .method = HTTP_GET, .handler = get_css_handler, .user_ctx = server_buf};
    httpd_register_uri_handler(server, &get_style);

    httpd_uri_t get_script = {
        .uri = "/script.js", .method = HTTP_GET, .handler = get_js_handler, .user_ctx = server_buf};
    httpd_register_uri_handler(server, &get_script);

    httpd_uri_t redir_uri = {
        .uri = "/*", .method = HTTP_GET, .handler = index_redirect_handler, .user_ctx = server_buf};
    httpd_register_uri_handler(server, &redir_uri);

    httpd_uri_t set_offset_uri = {
        .uri = "/Offset", .method = HTTP_POST, .handler = set_offset_handler, .user_ctx = server_buf};
    httpd_register_uri_handler(server, &set_offset_uri);

    return ESP_OK;
}
