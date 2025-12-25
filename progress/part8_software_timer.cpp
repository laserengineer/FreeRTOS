#include <Arduino.h>
#include <stdint.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Globals for software timer demo
static TimerHandle_t one_shot_timer = NULL;
static TimerHandle_t auto_reload_timer = NULL;

//****************************************
// Callbacks

void myTimerCallback(TimerHandle_t xTimer)
{

    // Identify which timer expired by checking its ID
    // Print message if timer 0 expired
    if ((uint32_t)pvTimerGetTimerID(xTimer) == 0)
    {
        Serial.println("One-shot timer expired");
    }

    // Print message if timer 1 expired
    if ((uint32_t)pvTimerGetTimerID(xTimer) == 1)
    {
        Serial.println("Auto-reload timer expired");
    }
}

void setup()
{
    // No need to initialize the RGB LED
    Serial.begin(115200);

    // Wait a moment to start (so we don't miss Serial output)
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Timer Demo---");

    // Create a one-shot timer
    one_shot_timer = xTimerCreate(
        "OneShot Timer",     // Text name for the timer
        pdMS_TO_TICKS(3000), // Timer period in ticks (3 seconds)
        pdFALSE,             // No auto-reload
        (void *)0,           // Timer ID is 0
        myTimerCallback);    // Callback function

    // Create an auto-reload timer
    auto_reload_timer = xTimerCreate(
        "AutoReload Timer",  // Text name for the timer
        pdMS_TO_TICKS(2000), // Timer period in ticks (2 seconds)
        pdTRUE,              // Auto-reload
        (void *)1,           // Timer ID is 1
        myTimerCallback);    // Callback function

    // CHeck to make sure timers were created
    if (one_shot_timer == NULL || auto_reload_timer == NULL)
    {
        Serial.println("Failed to create timers");
    }
    else
    {
        // Wait and then print a message that we are starting the timers
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        Serial.println("Starting timers...");

        // Start the timers
        xTimerStart(one_shot_timer, portMAX_DELAY);
        xTimerStart(auto_reload_timer, portMAX_DELAY);
    }

    // delete self to show that timers will continue to run taks
    vTaskDelete(NULL);
}

void loop()
{
    // Nothing to do here - everything is done in timer callbacks
}