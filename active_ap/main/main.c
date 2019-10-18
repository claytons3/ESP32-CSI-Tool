#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "nvs_component.h"
#include "sd_component.h"
#include "csi_component.h"
#include "time_component.h"
#include "input_component.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

/*
 * The examples use WiFi configuration that you can set via 'make menuconfig'.
 *
 * If you'd rather not, just change the below entries to strings with
 * the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
 */
#define LEN_MAC_ADDR 20
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN       16

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "Active CSI collection (AP)";

static esp_err_t event_handler(void *ctx, system_event_t *event) {
    switch (event->event_id) {
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "station:"
            MACSTR
            " join, AID=%d",
                    MAC2STR(event->event_info.sta_connected.mac),
                    event->event_info.sta_connected.aid);
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "station:"
            MACSTR
            "leave, AID=%d",
                    MAC2STR(event->event_info.sta_disconnected.mac),
                    event->event_info.sta_disconnected.aid);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void softap_init() {
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
            .ap = {
                    .ssid = EXAMPLE_ESP_WIFI_SSID,
                    .password = EXAMPLE_ESP_WIFI_PASS,
                    .max_connection = 1, //EXAMPLE_MAX_STA_CONN,
                    .authmode = WIFI_AUTH_WPA_WPA2_PSK
            },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "softap_init finished. SSID:%s password:%s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

void free_ctx_func(void *ctx) {
    free(ctx);
}

esp_err_t get_handler(httpd_req_t *req) {
    if (!req->sess_ctx) {
        req->sess_ctx = malloc(sizeof(int));
        req->free_ctx = free_ctx_func;
    }

    /* Send the current (local) timestamp */
    char* resp = time_string_get();
    httpd_resp_send(req, resp, strlen(resp));
    free(resp);

    return ESP_OK;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = get_handler,
        .user_ctx = NULL
};


/* Function for starting the webserver */
httpd_handle_t webserver_init(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
    }

    return server;
}

int server, client;
#define NUM_RECV 101
char input[NUM_RECV + 1];

void webserver_loop() {
    uint32_t inet_len;
    struct sockaddr_in saddr, caddr;

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(2223);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    server = socket(PF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        printf("Socket creation error\n");
        return;
    }

    if (bind(server, (struct sockaddr *)&saddr, sizeof(struct sockaddr)) == -1) {
        printf("Socket bind error\n");
        return;
    }

    if (listen(server,5) == -1) {
        printf("Socket listen error\n");
        return;
    }

    while (1) {
        inet_len = sizeof(caddr);
        if ((client = accept(server, (struct sockaddr *)&caddr, &inet_len)) == -1) {
            printf("Client accept error\n");
            close(server);
            return;
        }
        printf("server new client connection [%s/%d]\n", inet_ntoa(caddr.sin_addr), caddr.sin_port);

        while (1) { // CONNECTED!?!?!?
            int count = read(client, &input, NUM_RECV);
//            input[count] = '\0';
//            printf("received a value of [%s]\n", input);
        }

//        // check if file exists
//        // @SEE http://stackoverflow.com/a/230068
//        value = ntohl(access(input, F_OK) != -1);

//        if (write(client, &value, sizeof(value)) != sizeof(value)) {
//            printf("Client accept error\n");
//            close(server);
//            return(-1);
//        }
//
//        if (htonl(value) == FILE_EXISTS) {
//            printf("File exists, starting transfer\n");
//            FILE *file = fopen(input, "r+");
//            while (fgets(input, 50, file) != NULL) {
//                write(client, &input, 50);
//            }
//            strcpy(input, "cmsc257");
//            write(client, &input, 50);
//            printf("Transfer complete\n\n");
//        } else {
//            printf("File does not exists, closing.\n\n");
//        }

//        close(client);
    }
}

void app_main() {
    nvs_init();
//    sd_init();
    softap_init();
    csi_init("AP");
//    webserver_init();
//    input_loop();
    webserver_loop();
}
