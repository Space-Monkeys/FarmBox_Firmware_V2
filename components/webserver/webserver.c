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

static esp_err_t write_new_config_file(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    httpd_resp_set_type(req, "application/json");

    char *body_complete_data;

    const char *initial_body = "";
    body_complete_data = malloc(2000);        /* make space for the new string (should check the return value ...) */
    strcpy(body_complete_data, initial_body); /* copy name into the new var */
    //strcat(body_complete_data, buf);          /* mount entire buff */
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

        strcat(body_complete_data, buf); /* mount entire buff */
    }

    cJSON *body_json = cJSON_Parse(body_complete_data);
    char *string = cJSON_Print(body_json);

    ESP_LOGI(WEBSEVER_INTERNAL, "%s", string);

    writeFile("/spiffs/default.json", string);

    httpd_resp_send_chunk(req, NULL, 0);
    free(body_complete_data);
    return ESP_OK;
}
static esp_err_t water_pump_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    httpd_resp_set_type(req, "application/json");

    char *body_complete_data;

    const char *initial_body = "";
    body_complete_data = malloc(2000);        /* make space for the new string (should check the return value ...) */
    strcpy(body_complete_data, initial_body); /* copy name into the new var */
    //strcat(body_complete_data, buf);          /* mount entire buff */
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

        strcat(body_complete_data, buf); /* mount entire buff */
    }

    cJSON *body_json = cJSON_Parse(body_complete_data);
    char *string = cJSON_Print(body_json);
    ESP_LOGI(WEBSEVER_INTERNAL, "%s", string);

    const cJSON *pump_activation_interval = NULL;
    pump_activation_interval = cJSON_GetObjectItemCaseSensitive(body_json, "pump_activation_interval");

    //writeFile("/spiffs/default.json", string);

    if (cJSON_IsString(pump_activation_interval) && (pump_activation_interval->valuestring != NULL))
    {
        ESP_LOGI(WEBSEVER_INTERNAL, "%s", pump_activation_interval->valuestring);

        editFile("/spiffs/default.json", "pump_activation_interval", pump_activation_interval->valuestring);
    }
    //char *file_new_default_config = cJSON_Print(json);
    //char *file_default_config = readFile("/spiffs/hello.txt");

    httpd_resp_send_chunk(req, NULL, 0);
    free(body_complete_data);
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
static const httpd_uri_t new_config_file = {
    .uri = "/config/file",
    .method = HTTP_POST,
    .handler = write_new_config_file,
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
        httpd_register_uri_handler(server, &new_config_file);

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
