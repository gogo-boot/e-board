#include "util/time_manager.h"
#include "util/sleep_utils.h"
#include <WiFi.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <time.h>

static const char* TAG = "SLEEP";

// Print wakeup reason after deep sleep
void printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      ESP_LOGI(TAG, "Wakeup caused by external signal using RTC_IO");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      ESP_LOGI(TAG, "Wakeup caused by external signal using RTC_CNTL");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      ESP_LOGI(TAG, "Wakeup caused by timer");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      ESP_LOGI(TAG, "Wakeup caused by touchpad");
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      ESP_LOGI(TAG, "Wakeup caused by ULP program");
      break;
    default:
      ESP_LOGI(TAG, "Wakeup was not caused by deep sleep: %d", wakeup_reason);
      break;
  }
}

// Calculate sleep time until next scheduled wakeup
uint64_t calculateSleepTime(int wakeupIntervalMinutes) {
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
  
  // Calculate next wakeup time based on interval
  int currentMinute = timeinfo->tm_min;
  int currentSecond = timeinfo->tm_sec;
  
  // Find next interval boundary (e.g., 0, 5, 10, 15, 20... for 5-min intervals)
  int nextWakeupMinute = ((currentMinute / wakeupIntervalMinutes) + 1) * wakeupIntervalMinutes;
  int minutesToSleep = nextWakeupMinute - currentMinute;
  int secondsToSleep = (minutesToSleep * 60) - currentSecond;
  
  // Handle hour boundary
  if (nextWakeupMinute >= 60) {
    secondsToSleep = (60 - currentMinute) * 60 - currentSecond;
  }
  
  ESP_LOGI(TAG, "Current: %02d:%02d:%02d, next wakeup in %d seconds (at xx:%02d:00)", 
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, 
           secondsToSleep, nextWakeupMinute % 60);
  
  return (uint64_t)secondsToSleep * 1000000ULL; // Convert to microseconds
}

// Calculate sleep time until specific time (e.g., 01:00)
uint64_t calculateSleepUntilTime(int targetHour, int targetMinute) {
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
  
  // Create target time structure
  struct tm targetTime = *timeinfo;
  targetTime.tm_hour = targetHour;
  targetTime.tm_min = targetMinute;
  targetTime.tm_sec = 0;
  
  time_t targetTimestamp = mktime(&targetTime);
  
  // If target time is in the past today, set it for tomorrow
  if (targetTimestamp <= now) {
    targetTime.tm_mday += 1;
    targetTimestamp = mktime(&targetTime);
  }
  
  int secondsToSleep = (int)(targetTimestamp - now);
  
  ESP_LOGI(TAG, "Current: %02d:%02d:%02d, sleeping until %02d:%02d:00 (%d seconds)", 
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
           targetHour, targetMinute, secondsToSleep);
  
  return (uint64_t)secondsToSleep * 1000000ULL;
}

// Enhanced deep sleep function with multiple wakeup options
void enterDeepSleep(uint64_t sleepTimeUs) {
  if (sleepTimeUs <= 0) {
      ESP_LOGE(TAG, "Invalid sleep time: %llu, not entering deep sleep!", sleepTimeUs);
      return;
  }

  // Setup time synchronization after WiFi connection
  if (!TimeManager::isTimeSet()) {
      ESP_LOGI(TAG, "Time not set, synchronizing with NTP...");
      TimeManager::setupNTPTime();

      ESP_LOGI(TAG, "Entering deep sleep for %llu microseconds (%.1f minutes)", 
              sleepTimeUs, sleepTimeUs / 60000000.0);
      
      // Configure timer wakeup
      esp_sleep_enable_timer_wakeup(sleepTimeUs);
      
      // Optional: Configure GPIO wakeup (e.g., for user button)
      // esp_sleep_enable_ext0_wakeup(GPIO_NUM_9, 0); // Wake on low signal on GPIO 9
      
      // // Power down WiFi and Bluetooth
      // WiFi.disconnect(true);
      // WiFi.mode(WIFI_OFF);
      
      // Flush serial output
      Serial.flush();
      
      // Enter deep sleep
      esp_deep_sleep_start();
  }

}