// ISR Interrupt Service Routine (ISR) example using ESP32 hardware timer (Arduino-ESP32 3.x style)

#include <Arduino.h>
#include <BoardConfig.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Settings
static const uint32_t timer_frequency_hz = 1000000; // 1 MHz timer tick (1 us per tick)
static const uint32_t timer_max_count = 1000000;    // 1,000,000 us = 1 second

// Globals
static hw_timer_t *timer = nullptr;
static SemaphoreHandle_t timerSem = nullptr;

// --- Shared Variables ---
// 'volatile' is mandatory for variables modified inside an ISR
volatile uint32_t timerCount = 0;
volatile bool ledstate = false;

// --- Spinlock for thread safety ---
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//*****************************************************************************
// Interrupt Service Routine (keep it FAST)

void IRAM_ATTR onTimer()
{
    // 1. Enter Critical Section (protect shared variables)
    portENTER_CRITICAL_ISR(&timerMux);
    timerCount = timerCount + 1;
    portEXIT_CRITICAL_ISR(&timerMux);

    // 2. Use a semaphore to tell a task to do the "heavy lifting" (like printing) [7, 8]
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(timerSem, &xHigherPriorityTaskWoken);

    // 3. Force the scheduler to run immediately if a higher priority task was woken [8]
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

// *****************************************************************************
// Task to handle slow actions (signaled by the ISR)
void timerTask(void *pvParameters)
{

    uint32_t currentCount;
    // Wait for signal from ISR

    while (true)
    {
        // Wait for signal from ISR
        if (xSemaphoreTake(timerSem, portMAX_DELAY) == pdTRUE)
        {
            // 1. Read the counter safely inside a critical section
            // We make a local copy so the number doesn't change while we are printing it
            portENTER_CRITICAL(&timerMux);
            currentCount = timerCount;
            ledstate = !ledstate; // Toggle LED state
            portEXIT_CRITICAL(&timerMux);

            // 2. Do the heavy I/O work outside the critical section
            digitalWrite(LED_BUILTIN, ledstate ? HIGH : LOW);

            Serial.print("Timer Count: ");
            Serial.print(currentCount);
            Serial.print(" | LED: ");
            Serial.println(ledstate ? "ON" : "OFF");
        }
    }
}

//*****************************************************************************
// Main

void setup()
{
    Serial.begin(115200);
    vTaskDelay(pdMS_TO_TICKS(1000));

    Serial.println("---CPU Timer---");
    Serial.print("CPU MHz: ");
    Serial.println(getCpuFrequencyMhz());

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    timerSem = xSemaphoreCreateBinary();
    if (timerSem == nullptr)
    {
        Serial.println("Failed to create semaphore");
        while (true)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    timer = timerBegin(timer_frequency_hz);
    if (timer == nullptr)
    {
        Serial.println("Failed to init timer");
        while (true)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    timerAttachInterrupt(timer, &onTimer);
    timerWrite(timer, 0);
    timerAlarm(timer, timer_max_count, true, 0);

    xTaskCreatePinnedToCore(
        timerTask,    // Task function
        "Timer Task", // Name of the task (for debugging)
        4096,         // Stack size (bytes)
        nullptr,      // Task input parameter
        1,            // Priority of the task
        nullptr,      // Task handle
        app_cpu);     // Run on APP CPU
}

void loop()
{
}
