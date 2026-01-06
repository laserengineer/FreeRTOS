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
static const uint32_t timer_frequency_hz = 1000000; // 1 MHz timer tick (1 us per tick)
static const uint32_t timer_max_count = 1000000;    // 1,000,000 us = 1 second

// Pins
static const int adc_pin = A0; // GPIO36, ADC1_CH0

// Globals
static hw_timer_t *timer = NULL;
static volatile uint16_t val;
static SemaphoreHandle_t bin_sem = NULL;

//*****************************************************************************
// Interrupt Service Routines (ISRs)

void IRAM_ATTR onTimer()
{
    BaseType_t task_woken = pdFALSE;

    // Perform ADC reading
    val = analogRead(adc_pin);

    // Give semaphore to wake up task
    xSemaphoreGiveFromISR(bin_sem, &task_woken);
    if (task_woken)
    {
        portYIELD_FROM_ISR();
    }
}

// *****************************************************************************
// Task to handle slow actions (signaled by the ISR)
void printValue(void *pvParameters)
{

    while (1)
    {
        // Wait for signal from ISR
        if (xSemaphoreTake(bin_sem, portMAX_DELAY) == pdTRUE)
        {
            // Print the ADC value
            Serial.print("ADC Value: ");
            Serial.println(val);
        }
    }
}

//*****************************************************************************
// Main

void setup()
{
    Serial.begin(115200);
    vTaskDelay(pdMS_TO_TICKS(1000));

    Serial.println("---FreeRTOS ISR Buffer Demo---");

    // Create binary semaphore
    bin_sem = xSemaphoreCreateBinary();
    if (bin_sem == NULL)
    {
        Serial.println("Failed to create semaphore");
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    // Create task to print ADC value
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

    // Enable Timer to work in autoreload mode
    timerAlarm(timer, timer_max_count, true, 0);

    // Delete "Setup and loop" task
    vTaskDelete(NULL);
}

void loop()
{
}
