#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <Arduino.h>
#include <esp_http_client.h>

// OTA Configuration
#define FIRMWARE_VERSION    0.1
#define UPDATE_JSON_URL     "https://raw.githubusercontent.com/gogo-boot/e-board/refs/heads/61-firmware-ota-update/test/ota/example.json"

// External variables
extern char rcv_buffer[200];
extern const char server_cert_pem_start[] asm("_binary_cert_github_pem_start");
extern const char server_cert_pem_end[] asm("_binary_cert_github_pem_end");

// Function declarations
esp_err_t _http_event_handler(esp_http_client_event_t* evt);
void check_update_task(void* pvParameter);

#endif // OTA_UPDATE_H
