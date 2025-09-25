#include "ota/ota_update.h"
#include <Arduino.h>
#include <WiFi.h>
#include "cJSON.h"
#include "util/wifi_manager.h"
#include <esp_https_ota.h>

static const char* TAG = "OTA_UPDATE";

// receive buffer
char rcv_buffer[200];

// Declare the certificate
extern const char server_cert_pem_start[] asm("_binary_cert_github_pem_start");
extern const char server_cert_pem_end[] asm("_binary_cert_github_pem_end");

// esp_http_client event handler
esp_err_t _http_event_handler(esp_http_client_event_t* evt) {
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        break;
    case HTTP_EVENT_ON_CONNECTED:
        break;
    case HTTP_EVENT_HEADER_SENT:
        break;
    case HTTP_EVENT_ON_HEADER:
        break;
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client)) {
            strncpy(rcv_buffer, (char*)evt->data, evt->data_len);
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        break;
    case HTTP_EVENT_DISCONNECTED:
        break;
    }
    return ESP_OK;
}

// Check update task
// downloads every 30sec the json file with the latest firmware
void check_update_task(void* pvParameter) {
    while (1) {
        // Connect to WiFi in station mode
        MyWiFiManager::reconnectWiFi();
        // Check if WiFi is connected before attempting HTTP request
        if (WiFi.status() != WL_CONNECTED) {
            ESP_LOGW(TAG, "WiFi not connected, skipping OTA check");
            vTaskDelay(30000 / portTICK_PERIOD_MS);
            continue;
        }

        printf("Looking for a new firmware...\n");

        // configure the esp_http_client
        esp_http_client_config_t config = {
            .url = UPDATE_JSON_URL,
            .cert_pem = server_cert_pem_start,
            // .cert_len = server_cert_pem_end - server_cert_pem_start,
            .timeout_ms = 15000,
            .event_handler = _http_event_handler,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);

        // downloading the json file
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            // parse the json file
            cJSON* json = cJSON_Parse(rcv_buffer);
            if (json == NULL) printf("downloaded file is not a valid json, aborting...\n");
            else {
                cJSON* version = cJSON_GetObjectItemCaseSensitive(json, "version");
                cJSON* file = cJSON_GetObjectItemCaseSensitive(json, "file");

                // check the version
                if (!cJSON_IsNumber(version)) printf("unable to read new version, aborting...\n");
                else {
                    double new_version = version->valuedouble;
                    if (new_version > FIRMWARE_VERSION) {
                        printf("current firmware version (%.1f) is lower than the available one (%.1f), upgrading...\n",
                               FIRMWARE_VERSION, new_version);
                        if (cJSON_IsString(file) && (file->valuestring != NULL)) {
                            printf("downloading and installing new firmware (%s)...\n", file->valuestring);
                            esp_http_client_config_t ota_client_config = {
                                .url = file->valuestring,
                                .cert_pem = server_cert_pem_start,
                                // .cert_len = server_cert_pem_end - server_cert_pem_start,
                                // .timeout_ms = 15000,
                                .event_handler = _http_event_handler,
                            };
                            esp_err_t ret = esp_https_ota(&ota_client_config);
                            if (ret == ESP_OK) {
                                printf("OTA OK, restarting...\n");
                                esp_restart();
                            } else {
                                printf("OTA failed...\n");
                            }
                        } else printf("unable to read the new file name, aborting...\n");
                    } else
                        printf(
                            "current firmware version (%.1f) is greater or equal to the available one (%.1f), nothing to do...\n",
                            FIRMWARE_VERSION, new_version);
                }
            }
        } else printf("unable to download the json file, aborting...\n");

        // cleanup
        esp_http_client_cleanup(client);

        printf("\n");
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}
