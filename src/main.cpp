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
static const TickType_t dim_delay = 5000 / portTICK_PERIOD_MS;               // Time before dimming LED
static const TickType_t blink_interval = 200 / portTICK_PERIOD_MS;           // LED blink interval
static const TickType_t remain_display_interval = 1000 / portTICK_PERIOD_MS; // How often to print remaining time

// Globals
static TimerHandle_t led_timer = NULL;
static TaskHandle_t led_blink_task_handle = NULL;
static TaskHandle_t remain_time_task_handle = NULL;
static TickType_t timer_end_tick = 0;

//*****************************************************************************
// Callbacks

// Turn off LED when timer expires and stop blinking task
void ledTimerCallback(TimerHandle_t xTimer)
{
    // Stop the blinking task
    if (led_blink_task_handle != NULL)
    {
        vTaskDelete(led_blink_task_handle);
        led_blink_task_handle = NULL;
    }

    // Stop the remaining time display task
    if (remain_time_task_handle != NULL)
    {
        vTaskDelete(remain_time_task_handle);
        remain_time_task_handle = NULL;
    }

    digitalWrite(LED_PIN, LOW);
    Serial.println("LED dimmed OFF after 5 second delay");
}

//*****************************************************************************
// Tasks

void ledBlinkTask(void *parameters)
{
    pinMode(LED_PIN, OUTPUT);
    bool ledstate = false;
    while (1)
    {
        ledstate = !ledstate;
        digitalWrite(LED_PIN, ledstate ? HIGH : LOW);
        vTaskDelay(blink_interval);
    }
}

void remainTimeTask(void *parameters)
{
    while (1)
    {
        TickType_t now = xTaskGetTickCount();
        TickType_t remain = (timer_end_tick > now) ? (timer_end_tick - now) : 0;
        Serial.print("Timer remaining: ");
        Serial.print(remain * portTICK_PERIOD_MS);
        Serial.println(" ms");
        if (remain == 0)
        {
            // Timer expired, exit task
            break;
        }
        vTaskDelay(remain_display_interval);
    }
    remain_time_task_handle = NULL;
    vTaskDelete(NULL);
}

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
            // Start Timer (if timer is already running, reset it )
            xTimerStart(led_timer, portMAX_DELAY);
            // Calculate and store timer end tick
            timer_end_tick = xTaskGetTickCount() + dim_delay;
            // timer_end_tick = xTimerGetExpiryTime(led_timer);

            // Start or restart the blink task
            if (led_blink_task_handle == NULL)
            {
                xTaskCreatePinnedToCore(
                    ledBlinkTask,           // Task function
                    "LED Blink",            // Name of task
                    2048,                   // Stack size in words
                    NULL,                   // Task input parameter
                    1,                      // Priority of the task
                    &led_blink_task_handle, // Task handle
                    app_cpu);               // Core where the task should run
            }

            // Start or restart the remaining time display task
            if (remain_time_task_handle != NULL)
            {
                vTaskDelete(remain_time_task_handle);
                remain_time_task_handle = NULL;
            }
            xTaskCreatePinnedToCore(
                remainTimeTask,
                "Remain Time Task",
                1024,
                NULL,
                2,
                &remain_time_task_handle,
                app_cpu);
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