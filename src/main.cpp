/*
 * MyStation E-Board - ESP32-C3 Public Transport Departure Board
 *
 * Boot Process Flow:
 * 1. System starts and calls DeviceModeManager::hasValidConfiguration()
 * 2. If no valid config -> DeviceModeManager::runConfigurationMode()
 *    - Creates WiFi hotspot for setup
 *    - Starts web server for user configuration
 *    - Stays awake to handle configuration
 * 3. If valid config exists -> DeviceModeManager::runOperationalMode()
 *    - Connects to saved WiFi
 *    - Fetches transport and weather data
 *    - Updates display and enters deep sleep
 *
 * Configuration persists in NVS (Non-Volatile Storage) across:
 * - Power loss/battery changes
 * - Firmware updates
 * - Manual resets
 */

#define ARDUINOJSON_DECODE_NESTING_LIMIT 1000
#include <Arduino.h>
#include <WebServer.h>
#include <esp_log.h>
#include <esp_https_ota.h>
#include <esp_http_client.h>
#include "config/config_struct.h"
#include "config/config_manager.h"
#include "util/device_mode_manager.h"
#include <SPI.h>
//EPD
#include "config/pins.h"

// GxEPD2 display library includes for GDEY075T7 (800x480)
//IO settings
//SCLK--GPIO18
//MOSI--GPIO23
#define isEPD_W21_BUSY digitalRead(Pins::EPD_BUSY)  //BUSY
#define EPD_W21_RST_0 digitalWrite(Pins::EPD_RES,LOW)  //RES
#define EPD_W21_RST_1 digitalWrite(Pins::EPD_RES,HIGH)
#define EPD_W21_DC_0  digitalWrite(Pins::EPD_DC,LOW) //DC
#define EPD_W21_DC_1  digitalWrite(Pins::EPD_DC,HIGH)
#define EPD_W21_CS_0 digitalWrite(Pins::EPD_CS,LOW) //CS
#define EPD_W21_CS_1 digitalWrite(Pins::EPD_CS,HIGH)

#define EPD_WIDTH   800
#define EPD_HEIGHT  480
#define EPD_ARRAY  EPD_WIDTH*EPD_HEIGHT/8
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>  // Specific driver for GDEY075T7

#include "util/wifi_manager.h"

// Create display instance for GDEY075T7 (800x480 resolution)
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display(
    GxEPD2_750_GDEY075T7(Pins::EPD_CS, Pins::EPD_DC, Pins::EPD_RES, Pins::EPD_BUSY));

// Create U8g2 instance for UTF-8 font support (for German umlauts)
U8G2_FOR_ADAFRUIT_GFX u8g2;

static const char* TAG = "MAIN";

// --- Globals (shared across modules) ---
WebServer server(80);
RTC_DATA_ATTR unsigned long loopCount = 0;
RTC_DATA_ATTR bool hasValidConfig = false; // Flag to track if valid config exists

// This Struct is only for showing on configureation web interface
// It is used to hold dynamic data like stopNames, stopIds, and stopDistances from API calls
// This will not be used to store configuration data in NVS
ConfigOption g_webConfigPageData;
#define FIRMWARE_VERSION	0.1
#define UPDATE_JSON_URL		"https://raw.githubusercontent.com/gogo-boot/e-board/refs/heads/61-firmware-ota-update/test/ota/example.json"
// receive buffer
char rcv_buffer[200];
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

// Declare the certificate
// The _binary_certs_pem_start follows this naming pattern:
// _binary_ - Prefix added by build system for embedded files
// certs_pem - Your filename certs.pem with dots converted to underscores
// _start/_end - Markers for file boundaries
extern const char server_cert_pem_start[] asm("_binary_cert_github_pem_start");
extern const char server_cert_pem_end[] asm("_binary_cert_github_pem_end");

#include "cJSON.h"
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


void setup() {
    Serial.begin(115200);
    delay(1000);
    // Allow time for serial monitor to connect, only for local debugging, todo remove in production or activate by flag

    // esp_log_level_set("*", ESP_LOG_DEBUG); // Set global log level
    ESP_LOGI(TAG, "System starting...");

    // TODO: Add any additional display initialization code here

    // Determine device mode based on saved configuration
    if (hasValidConfig || DeviceModeManager::hasValidConfiguration(hasValidConfig)) {
        // Run operational mode - choose one of the following:
        DeviceModeManager::showWeatherDeparture();
        // DeviceModeManager::showGeneralWeather();
        // DeviceModeManager::showDeparture();

        // After operational mode completes, enter deep sleep
        DeviceModeManager::enterOperationalSleep();

        // start the check update task
        // check_update_task(NULL);
        // xTaskCreate(&check_update_task, "check_update_task", 8192, NULL, 5, NULL);
    } else {
        DeviceModeManager::runConfigurationMode();
    }
}

void loop() {
    // Only handle web server in config mode
    if (ConfigManager::isConfigMode()) {
        server.handleClient();
        delay(10); // Small delay to prevent watchdog issues
    } else {
        // Normal operation happens in setup() and then device goes to sleep
        // This should never be reached in normal operation
        ESP_LOGW(TAG, "Unexpected: loop() called in normal operation mode");
        delay(5000);
    }
}
