#include <stdio.h>
#include "filesystem.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include <esp_event.h>

static const char *TAG_FILESYSTEM = "FILESYSTEM";

void filesystem_init()
{
    //############### FILESYSTEM ####################

    ESP_LOGI(TAG_FILESYSTEM, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG_FILESYSTEM, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG_FILESYSTEM, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG_FILESYSTEM, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_FILESYSTEM, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG_FILESYSTEM, "Partition size: total: %d, used: %d", total, used);
    }
}

const char *getFiles(char *filepath)
{
    return filepath;
}
