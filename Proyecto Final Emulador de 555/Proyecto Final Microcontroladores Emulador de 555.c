#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

#include "driver/gpio.h"
#include "driver/ledc.h"

#define LED_GPIO 2

static const char *TAG = "NE555_EMU";

// HTML con selección de R, C y botones
const char *html_index = "<!DOCTYPE html><html><head><title>NE555 Sim</title></head><body style='font-family:sans-serif;text-align:center;'>"
"<h1>Simulador NE555 - Modo Astable</h1>"
"<form action=\"/start\">"
"<label>Resistencia R1 (Ω):</label><select name='r1'>"
"<option value='50'>50</option><option value='75'>75</option><option value='100'>100</option>"
"</select><br><br>"
"<label>Resistencia R2 (Ω):</label><select name='r2'>"
"<option value='50'>50</option><option value='75'>75</option><option value='100'>100</option>"
"</select><br><br>"
"<label>Capacitor C (uF):</label><select name='c'>"
"<option value='4.7'>4.7</option><option value='10'>10</option><option value='47'>47</option><option value='100'>100</option><option value='470'>470</option>"
"</select><br><br>"
"<button type='submit' style='font-size:20px;'>EJECUTAR</button>"
"</form><br>"
"<a href='/stop'><button style='font-size:20px;'>DETENER</button></a>"
"</body></html>";

bool pwm_running = false;

void start_pwm(double freq, double duty) {
    ledc_timer_config_t timer = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = (uint32_t)freq,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .channel    = LEDC_CHANNEL_0,
        .duty       = (uint32_t)((1023 * duty) / 100.0),
        .gpio_num   = LED_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_sel  = LEDC_TIMER_0
    };
    ledc_channel_config(&channel);
    pwm_running = true;
}

void stop_pwm() {
    ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
    pwm_running = false;
}

esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_index, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t start_handler(httpd_req_t *req) {
    char buf[100];
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        httpd_req_get_url_query_str(req, buf, buf_len);

        char param[10];
        int r1 = 0, r2 = 0;
        float c = 0;

        if (httpd_query_key_value(buf, "r1", param, sizeof(param)) == ESP_OK)
            r1 = atoi(param);
        if (httpd_query_key_value(buf, "r2", param, sizeof(param)) == ESP_OK)
            r2 = atoi(param);
        if (httpd_query_key_value(buf, "c", param, sizeof(param)) == ESP_OK)
            c = atof(param);

        float t1 = 0.693 * (r1 + r2) * c;
        float t2 = 0.693 * r2 * c;
        float T = t1 + t2;
        float freq = 1.0 / T;
        float duty = (t1 / T) * 100.0;

        ESP_LOGI(TAG, "R1: %d Ω, R2: %d Ω, C: %.1f uF → Freq: %.2f Hz, PWM: %.1f%%", r1, r2, c, freq, duty);
        start_pwm(freq, duty);
    }

    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

esp_err_t stop_handler(httpd_req_t *req) {
    stop_pwm();
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_get_handler,
            .user_ctx = NULL
        };
        httpd_uri_t start_uri = {
            .uri = "/start",
            .method = HTTP_GET,
            .handler = start_handler,
            .user_ctx = NULL
        };
        httpd_uri_t stop_uri = {
            .uri = "/stop",
            .method = HTTP_GET,
            .handler = stop_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &start_uri);
        httpd_register_uri_handler(server, &stop_uri);
    }
    return server;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "Cliente conectado");
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        ESP_LOGI(TAG, "Cliente desconectado");
    }
}

void wifi_init_softap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "brazoban",
            .ssid_len = strlen("brazoban"),
            .channel = 1,
            .password = "12345678",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = true,
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP inicializado. SSID: NE555_SIM");
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    wifi_init_softap();
    start_webserver();
}

