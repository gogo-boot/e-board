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
