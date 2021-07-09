#include <stdio.h>
#include "webserver.h"
#include "esp_log.h"
#include <esp_http_server.h>
#include "cJSON.h"
#include <esp_event.h>
#include <sys/param.h>
#include "filesystem.h"

static const char *WEBSEVER_INTERNAL = "WEBSERVER";

/* Simple handler for getting system handler */

//Services

//Controllers
static esp_err_t system_info_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    cJSON_AddNumberToObject(root, "revision", chip_info.revision);
    cJSON_AddNumberToObject(root, "features", chip_info.features);
    cJSON_AddNumberToObject(root, "model", chip_info.model);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}
static esp_err_t water_pump_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0)
    {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                                  MIN(remaining, sizeof(buf)))) <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        char *string = NULL;

        cJSON *json_data = cJSON_Parse(buf);
        string = cJSON_Print(json_data);

        //printf("%s", string);

        //writeFile("/spiffs/hello.txt", string);
        readFile("/spiffs/hello.txt");
        /* Log data received */
        /* ESP_LOGI(WEBSEVER_INTERNAL, "=========== RECEIVED DATA ==========");
        ESP_LOGI(WEBSEVER_INTERNAL, "%.*s", ret, buf);
        ESP_LOGI(WEBSEVER_INTERNAL, "===================================="); */
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

//Routes
static const httpd_uri_t info = {
    .uri = "/info",
    .method = HTTP_GET,
    .handler = system_info_get_handler,
    .user_ctx = NULL};

static const httpd_uri_t pump = {
    .uri = "/pump",
    .method = HTTP_POST,
    .handler = water_pump_post_handler,
    .user_ctx = NULL};

static httpd_handle_t run_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(WEBSEVER_INTERNAL, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(WEBSEVER_INTERNAL, "Registering URI handlers");
        httpd_register_uri_handler(server, &info);
        httpd_register_uri_handler(server, &pump);
        return server;
    }

    ESP_LOGI(WEBSEVER_INTERNAL, "Error starting server!");
    return NULL;
}
void start_webserver(void)
{
    ESP_LOGI(WEBSEVER_INTERNAL, "========================== Starting Webserver ==========================");

    /* Start the server for the first time */
    static httpd_handle_t server = NULL;
    server = run_webserver();
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        ESP_LOGI(WEBSEVER_INTERNAL, "Starting webserver");
        *server = run_webserver();
    }
}
