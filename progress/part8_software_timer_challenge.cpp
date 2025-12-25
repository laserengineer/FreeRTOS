#include <Arduino.h>
#include <BoardConfig.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#define LED_PIN LED_BUILTIN // Use the built-in LED pin

// Settings
static const TickType_t dim_delay = 5000 / portTICK_PERIOD_MS; // Time before dimming LED

// Globals
static TimerHandle_t led_timer = NULL;

//*****************************************************************************
// Callbacks

// Turn off LED when timer expires
void ledTimerCallback(TimerHandle_t xTimer)
{
    digitalWrite(LED_PIN, LOW);
    Serial.println("");
    Serial.println("LED dimmed OFF after delay");
}

//*****************************************************************************
// Tasks

void doCLI(void *parameters)
{
    char c;
    pinMode(LED_PIN, OUTPUT);

    while (1)
    {
        // Check for serial input
        if (Serial.available() > 0)
        {
            // Echo everthing back to the serial port
            c = Serial.read();
            Serial.print(c);

            // Turn ON the LED
            digitalWrite(LED_PIN, HIGH);

            // Start Timer (if timer is already running, reset it )
            xTimerStart(led_timer, portMAX_DELAY);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // Yield to other tasks
    }
}

void setup()
{
    // No need to initialize the RGB LED
    Serial.begin(115200);
    // Wait a moment to start (so we don't miss Serial output)
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Timer Solution---");

    // Create a one-shot timer
    led_timer = xTimerCreate(
        "LED Dimmer",      // Text name for the timer
        dim_delay,         // Timer period in ticks
        pdFALSE,           // No auto-reload
        (void *)0,         // Timer ID
        ledTimerCallback); // Callback function

    if (led_timer == NULL)
    {
        Serial.println("Failed to create LED timer!");
        while (1)
            ; // Halt
    }

    // Create CLI task command line interface task
    xTaskCreatePinnedToCore(
        doCLI,      // Task function
        "CLI Task", // Name of task
        4096,       // Stack size in words
        NULL,       // Task input parameter
        1,          // Priority of the task
        NULL,       // Task handle
        app_cpu);   // Core where the task should run

    // Delete "setup and loop" task
    vTaskDelete(NULL);
}

// the loop function runs over and over again forever
void loop()
{
}