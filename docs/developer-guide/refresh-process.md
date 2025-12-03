## Display Refresh and Sleep Process

Boot Process and Sleep Duration is working tightly together. it runs in different phases and cycles. each of them
must ensure only its own process responsibility. If it is not ensured, it may cause unexpected behavior like missing
data update or excessive power consumption.

- Boot Process runs when device wakes up from sleep. which is current-time based process. When it wakes up, it
  checks the current display mode and decides which data to fetch and update the display.
- _Sleep Duration_ is calculated based on current display mode. Current Display mode is not always the same display mode
  of when it wakes-up in the future. so there are some edge cases to consider.

### Sleep Duration Calculation

1. It gets the current display mode from configuration.
    - It doesn't reflect temporary display mode.
    - If the display mode is "weather-only", it returns _weather-only_ display mode.
    - If the display mode is "transport-only", it returns _transport-only_ display mode
    - If the display mode is "half-and-half", it does reflect weekdays and weekends transport active time range.
      if it is in transport active time range, it returns _half-and-half_ display mode.
      if it is not in transport active time range, it returns _weather-only_ display mode.
1. It gets next required wake-up time according to current display mode.
    - If the display mode is "weather-only", it gets next weather update time.
    - If the display mode is "transport-only", it gets next transport update time.
    - If the display mode is "half-and-half", it gets both next weather and transport update time. and it returns which
      is sooner.
1. It doesn't always get desired wake-up time due to edge cases:
    - If the display mode is "half-and-half", it gets next transport wakes-up time. but it can be out of transport
      active time range. In this case, it should get next weather update time.
    - regardless any display mode, it can get wake-up time which falls in deep-sleep time range, which is not desired.
      In this case, it should get the end of deep-sleep time range as wake-up time.
    - regardless any display mode, the OTA Update may be scheduled regardless deep-sleep time range. in this case, it
      should get the OTA Update time as wake-up time.
1. One more thing to consider. there is temporary view mode. If temporary mode is active, it returns current time +
   temporary display time which is usually 2 minutes. so it wakes up soon to restore the configured display mode.

### Boot Process Flow

```mermaid
flowchart TD
    PrepareToSleep[Hibernate Display and Wifi Modem]
%%  Check Temporary Mode First
    Start([Wake Up]) --> CheckTempMode{{Temporary Mode\nActive?}}
    CheckTempMode -->|Yes| GetTempDisplayMode[Get Temporary Display Mode]
    CheckTempMode -->|No| GetConfiguredDisplayMode[Get Configured Display Mode]
%%  Temporary Mode Paths - Always fetch data
    GetTempDisplayMode -->|Weather Only| GetWeather[Get Weather Data]
    GetTempDisplayMode -->|Departure Only| GetDeparture[Get Departure Data]
    GetTempDisplayMode -->|Half & Half| TempCheckActive{{Departure in\nActive Time?}}
    TempCheckActive -->|Yes| GetWeatherAndDeparture[Get Weather & Departure Data]
    TempCheckActive -->|No| GetWeather
%%  Configured Mode Paths - Check if update needed first
    GetConfiguredDisplayMode -->|Weather Departure halfhalf Screen Mode| IfDepartureActive{{Departure in\nActive Time?}}
    GetConfiguredDisplayMode -->|Weather Full Screen Mode| CheckWeatherNeeded{{Time to update\nWeather?}}
    GetConfiguredDisplayMode -->|Departure Full Screen Mode| CheckDepartureNeeded{{Time to update\nDeparture?}}
    CheckWeatherNeeded -->|Yes| GetWeather
    CheckWeatherNeeded -->|No| PrepareToSleep
    CheckDepartureNeeded -->|Yes| GetDeparture
    CheckDepartureNeeded -->|No| PrepareToSleep
%%  Half Half Mode / If Departure is in Active Time
    IfDepartureActive -->|Yes| IfNeedWeatherDeparture{{Time to update Weather \n && Time to update Departure}}
    IfDepartureActive -->|No| IfNeedWeatherUpdate{{Time to update Weather}}
%%  If Departure is in Active Time / If Time to update Weather & Departure
    IfNeedWeatherDeparture -->|Yes| GetWeatherAndDeparture
    IfNeedWeatherDeparture -->|No| IfNeedDepartureUpdate{{Time to update Departure}}
%%  If Time to update Weather & Departure / If Time to update Departure
    IfNeedDepartureUpdate -->|Yes| GetDeparture
    IfNeedDepartureUpdate -->|No| IfTimeToUpdateWeather{{Time to update Weather}}
%%  If Time to update Departure / If Time to update Weather
    IfTimeToUpdateWeather -->|Yes| GetWeather
    IfTimeToUpdateWeather -->|No| PrepareToSleep
%%  If Departure is in Active Time / Time to update Weather
    IfNeedWeatherUpdate -->|Yes| GetWeather
    IfNeedWeatherUpdate -->|No| PrepareToSleep
%%  Shared Display Logic
    GetWeather --> UpdateWeatherDisplay[Update Weather Display]
    GetDeparture --> UpdateDepartureDisplay[Update Departure Display]
    GetWeatherAndDeparture --> UpdateBothDisplays[Update Weather & Departure]
    UpdateWeatherDisplay --> PrepareToSleep
    UpdateDepartureDisplay --> PrepareToSleep
    UpdateBothDisplays --> PrepareToSleep
    PrepareToSleep --> CalculateSleepDuration[Calculate Sleep Duration\nBased on Configured Mode or Temp Mode]
    CalculateSleepDuration --> DeepSleep[Enter Deep Sleep]
    style Start fill: #99ff99
    style DeepSleep fill: #9999ff
    style CheckTempMode fill: #ffcc99
    style GetTempDisplayMode fill: #ffcc99
```

Pseudo Code for Sleep Preparation

```c++
function cacluateSleepDuration() {
    time currentTime = time.Now();
    time nextDepartureUpdate = 0;
    time nextWeatherUpdate = 0;
    time nextWakeupTime = 0;

    // Calculate next departure update time
    if (config.displayMode == 0 || config.displayMode == 2) { // half-and-half or departure-only
        time lastDepartureUpdate = getLastTransportUpdate();
        nextDepartureUpdate = lastDepartureUpdate + (config.transportInterval * 60); // minutes to seconds
    }

    // Calculate next weather update time
    if (config.displayMode == 0 || config.displayMode == 1) { // half-and-half or weather-only
        time lastWeatherUpdate = getLastWeatherUpdate();
        nextWeatherUpdate = lastWeatherUpdate + (config.weatherInterval * 3600); // hours to seconds
    }

    // Determine earliest required update
    if (nextDepartureUpdate > 0 && nextWeatherUpdate > 0) {
        // Both updates needed - wake up for the earliest one
        nextWakeupTime = min(nextDepartureUpdate, nextWeatherUpdate);
    } else if (nextDepartureUpdate > 0) {
        // Only departure update needed
        nextWakeupTime = nextDepartureUpdate;
    } else if (nextWeatherUpdate > 0) {
        // Only weather update needed
        nextWakeupTime = nextWeatherUpdate;
    } else {
        // Default fallback - wake up in 1 minute
        nextWakeupTime = currentTime + 60;
    }

    // Calculate sleep duration in seconds
    int sleepDurationSeconds = nextWakeupTime - currentTime;

    // Ensure minimum sleep time and maximum reasonable sleep time
    if (sleepDurationSeconds < 30) {
        sleepDurationSeconds = 30; // Minimum 30 seconds
    }

    if (isInConfiguredDeepSleepTime(sleepDurationSeconds)){
        sleepDurationSeconds = getConfiguredDeepSleepEndDurationSecond();
    }

    // Convert to microseconds for ESP32 deep sleep
    esp_deep_sleep(sleepDurationSeconds * 1000000ULL);
}
```
