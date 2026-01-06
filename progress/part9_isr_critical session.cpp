/**
 * ESP32 ISR Critical Section Demo
 *
 * Increment global variable in ISR.
 */

#include <Arduino.h>

// Only use core 1 for demo purposes

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Settings
static const uint32_t timer_frequency_hz = 10000000;            // 1 MHz timer tick (1 us per tick)
static const uint32_t timer_max_count = 2500000;                // 500,000 us = 500 ms
static const TickType_t task_delay = 2000 / portTICK_PERIOD_MS; // 2 second delay

// Globals
static hw_timer_t *timer = NULL;
static volatile int isr_counter = 0;
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

//*****************************************************************************
// Interrupt Service Routines (ISRs)

void IRAM_ATTR onTimer()
{
    portENTER_CRITICAL_ISR(&spinlock);
    isr_counter = isr_counter + 1;
    portEXIT_CRITICAL_ISR(&spinlock);
}

// *****************************************************************************
// Task to handle slow actions (signaled by the ISR)
void printValue(void *pvParameters)
{

    while (1)
    {
        while (isr_counter > 0)
        {
            Serial.println(isr_counter);
            portENTER_CRITICAL_ISR(&spinlock);
            isr_counter = isr_counter - 1;
            portEXIT_CRITICAL_ISR(&spinlock);
        }

        // Wait 2 seconds while ISR increase the counter
        vTaskDelay(task_delay);
    }
}

//*****************************************************************************
// Main

void setup()
{
    Serial.begin(115200);
    vTaskDelay(pdMS_TO_TICKS(1000));

    Serial.println("---FreeRTOS ISR Critical Section Demo---");
    xTaskCreatePinnedToCore(
        printValue,    // Task function
        "Print Value", // Name of the task (for debugging)
        4096,          // Stack size (bytes)
        NULL,          // Task input parameter
        1,             // Priority of the task
        NULL,          // Task handle
        app_cpu);      // Run on APP CPU

    // Create and start the hardware timer
    timer = timerBegin(timer_frequency_hz);
    if (timer == nullptr)
    {
        Serial.println("Failed to init timer");
        while (true)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // Attach onTimer function to hardware timer
    timerAttachInterrupt(timer, &onTimer);
    // timerWrite(timer, 0);
    timerAlarm(timer, timer_max_count, true, 0);
}

void loop()
{
}
