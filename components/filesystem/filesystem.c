#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include "filesystem.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include <esp_event.h>
#include <dirent.h>
#include "cJSON.h"

static const char *TAG_FILESYSTEM = "FILESYSTEM";

void filesystem_init()
{
    //############### FILESYSTEM ####################

    ESP_LOGI(TAG_FILESYSTEM, "========================== Starting SPIFFS ==========================");

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
    DIR *d;
    struct dirent *dir;
    d = opendir("/spiffs");

    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            printf("%s\n", dir->d_name);
        }

        closedir(d);
    }
    return filepath;
}
char *readFile(char *filepath)
{

    /* declare a file pointer */
    FILE *infile;
    char *buffer;
    long numbytes;

    /* open an existing file for reading */
    infile = fopen(filepath, "r");

    /* quit if the file does not exist */
    if (infile != NULL)
    {
        /* Get the number of bytes */
        fseek(infile, 0L, SEEK_END);
        numbytes = ftell(infile) + 1;

        /* reset the file position indicator to the beginning of the file */
        fseek(infile, 0L, SEEK_SET);

        /* grab sufficient memory for the buffer to hold the text */
        buffer = (char *)calloc(numbytes, sizeof(char));

        /* memory error */
        if (buffer == NULL)
        {
            ESP_LOGE(TAG_FILESYSTEM, "Memory error");
        }

        /* copy all the text into the buffer */
        fread(buffer, sizeof(char), numbytes, infile);
        fclose(infile);

        /* confirm we have read the file by outputing it to the console */
        //  printf("The file called contains this text\n\n%s", buffer);

        return buffer;

        //ESP_LOGE(TAG_FILESYSTEM, "%s", buffer);

        /* free the memory we used for the buffer */
    }
    else
    {
        ESP_LOGE(TAG_FILESYSTEM, "File in %s does not exist", filepath);
        return "";
    }
}

int writeFile(char *filepath, char *file_data)
{
    // First create a file.
    ESP_LOGI(TAG_FILESYSTEM, "Opening file");
    FILE *f = fopen(filepath, "wb");
    if (f == NULL)
    {
        ESP_LOGE(TAG_FILESYSTEM, "Failed to open file for writing");
        return 0;
    }
    fprintf(f, file_data);
    fclose(f);
    ESP_LOGI(TAG_FILESYSTEM, "File written");
    return 1;
}
void editFile(char *filepath, char *json_key, char *json_key_data)
{
    /* declare a file pointer */
    FILE *infile;
    char *buffer;
    long numbytes;
    const cJSON *json_data = NULL;
    cJSON *new_index_str = NULL;

    /* open an existing file for reading */
    infile = fopen(filepath, "r");

    /* quit if the file does not exist */
    if (infile == NULL)
    {
        ESP_LOGE(TAG_FILESYSTEM, "File in %s does not exist", filepath);
    }

    /* Get the number of bytes */
    fseek(infile, 0L, SEEK_END);
    numbytes = ftell(infile);

    /* reset the file position indicator to the beginning of the file */
    fseek(infile, 0L, SEEK_SET);

    /* grab sufficient memory for the buffer to hold the text */
    buffer = (char *)calloc(numbytes, sizeof(char));

    /* memory error */
    if (buffer == NULL)
    {
        ESP_LOGE(TAG_FILESYSTEM, "Memory error");
    }

    /* copy all the text into the buffer */
    fread(buffer, sizeof(char), numbytes, infile);
    fclose(infile);

    cJSON *file_json = cJSON_Parse(buffer);

    json_data = cJSON_GetObjectItemCaseSensitive(file_json, json_key);

    if (cJSON_IsString(json_data) && (json_data->valuestring != NULL))
    {
        char *file_old_default_config = cJSON_Print(file_json);
        ESP_LOGI(TAG_FILESYSTEM, "Value before change:\n %s", file_old_default_config);
        cJSON_GetObjectItemCaseSensitive(file_json, json_key)->valuestring = json_key_data;

        char *file_new_default_config = cJSON_Print(file_json);
        writeFile(filepath, file_new_default_config);
        ESP_LOGI(TAG_FILESYSTEM, "Value after change: \n %s", file_new_default_config);
    }
    else
    {
        ESP_LOGW(TAG_FILESYSTEM, "%s", "Not find this index in file, creating new index....");
        new_index_str = cJSON_CreateString(json_key_data);
        cJSON_AddItemToObject(file_json, json_key, new_index_str);
        char *file_new_default_config = cJSON_Print(file_json);
        writeFile(filepath, file_new_default_config);
    }
    free(buffer);
}