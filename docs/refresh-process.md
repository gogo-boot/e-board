### Boot Process Flow

```mermaid
flowchart TD
    PrepareToSleep[Hibernate Display and Wifi Modem]
%%  Getting Configured Display Mode
    Start([Wake Up]) --> StartOperation
    StartOperation([Start Opration Mode]) --> GetDisplayMode[Get Configured Display Mode]
    GetDisplayMode -->|Weather Departure halfhalf Screen Mode| IfDepartureActive{{If Departure in Active Time}}
    GetDisplayMode -->|Weather Full Screen Mode| GetWeather[Get Weather Data] --> UpdateWeather[Update Weather Full Display] --> PrepareToSleep
    GetDisplayMode -->|Departure Full Screen Mode| GetDeparture[Get Departure Data] --> UpdateDeparture[Update Departure Full Display] --> PrepareToSleep
%%  Half Half Mode / If Departure is in Active Time
    IfDepartureActive -->|yes| IfNeedWeatherDeparture{{Time to update Weather \n && Time to update Departure}}
    IfDepartureActive -->|no| IfNeedWeatherUpdate{{Time to update Weather}}
%%  If Departure is in Active Time / If Time to update Weather & Departure
    IfNeedWeatherDeparture -->|no| IfNeedDepartureUpdate{{Time to update Departure}}
    IfNeedWeatherDeparture -->|yes| GetWeatherDepartureData[Get weather data \n Get departure Data] --> DisplayWeatherDeparture[Update Weather & Departure] --> PrepareToSleep
%%  If Time to update Weather & Departure / If Time to update Departure
    IfNeedDepartureUpdate -->|no| IfTimeToUpdateWeather{{Time to update Weather}}
    IfNeedDepartureUpdate -->|yes| GetDepartureData[Get Departure Data] --> DisplayDepartureHalf[Update Departure Half] --> PrepareToSleep
%%  If Time to update Departure / If Time to update Weather
    IfTimeToUpdateWeather -->|yes| GetWeatherData[Get Weather Data] --> DisplayWeatherHalf[Update Weather Half] --> PrepareToSleep
    IfTimeToUpdateWeather -->|no| PrepareToSleep
%%  If Departure is in Active Time / Time to update Weather
    IfNeedWeatherUpdate -->|no| PrepareToSleep
    IfNeedWeatherUpdate -->|yes| GetWeather
    PrepareToSleep --> CalculateSleepDuration[Calculate Sleep Duration Based on Next Required Update]
    CalculateSleepDuration --> DeepSleep[Enter Deep Sleep]
    style Start fill: #99ff99
    style DeepSleep fill: #9999ff
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
