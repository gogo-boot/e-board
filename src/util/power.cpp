#include "power.h"
#include <Arduino.h>
#include "esp_sleep.h"

void enterHibernate(uint64_t sleep_us) {
    Serial.printf("Entering hibernate mode for %.2f minutes...\n", sleep_us / 60000000.0);
    esp_sleep_enable_timer_wakeup(sleep_us);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_deep_sleep_start();
    // The device will reset after wakeup
}
