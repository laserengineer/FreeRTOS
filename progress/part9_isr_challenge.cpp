#include <Arduino.h>

// Use core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// --------------------------------------------------------------------------
// Settings
// --------------------------------------------------------------------------
static const uint32_t timer_frequency_hz = 1000000; // 1 MHz timer tick (1 us per tick)
static const uint32_t timer_max_count = 100000;     // 100,000 us = 100 ms = 10 Hz Sample Rate

// ADC Settings
static const int adc_pin = A0; // Adjust pin as necessary for your board
#define ADC_BUF_SIZE 10

// --------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------
static hw_timer_t *timer = nullptr;

// Synchronization
static SemaphoreHandle_t timerSem = nullptr;
static SemaphoreHandle_t avg_sem = nullptr;

// Shared Data (Protected by Mutexes)
volatile uint32_t timerCount = 0;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint16_t adc_buf[ADC_BUF_SIZE] = {0};
volatile uint8_t adc_count = 0;
portMUX_TYPE adc_buf_mux = portMUX_INITIALIZER_UNLOCKED;

// --------------------------------------------------------------------------
// Interrupt Service Routine (ISR)
// --------------------------------------------------------------------------
void IRAM_ATTR onTimer()
{
    // 1. Increment Timer Count safely
    portENTER_CRITICAL_ISR(&timerMux);
    timerCount++;
    portEXIT_CRITICAL_ISR(&timerMux);

    // 2. Signal the Timer Task to run
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(timerSem, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

// --------------------------------------------------------------------------
// Task 1: Handle Timer Event (Print Count & Sample ADC)
// --------------------------------------------------------------------------
void timerTask(void *pvParameters)
{
    uint32_t currentCount;

    while (true)
    {
        // Wait for the ISR to signal us (every 100ms)
        if (xSemaphoreTake(timerSem, portMAX_DELAY) == pdTRUE)
        {
            // --- Read Timer Count ---
            portENTER_CRITICAL(&timerMux);
            currentCount = timerCount;
            portEXIT_CRITICAL(&timerMux);
            // --- Perform ADC Sampling ---
            uint16_t rawVal = analogRead(adc_pin);

            // Store in buffer safely
            portENTER_CRITICAL(&adc_buf_mux);
            if (adc_count < ADC_BUF_SIZE)
            {
                adc_buf[adc_count++ % ADC_BUF_SIZE] = rawVal;
            }

            // If buffer is full (10 samples), trigger the Average Task
            if (adc_count % ADC_BUF_SIZE == 0 && adc_count != 0)
            {
                xSemaphoreGive(avg_sem);
            }
            portEXIT_CRITICAL(&adc_buf_mux);
        }
    }
}

// --------------------------------------------------------------------------
// Task 2: Calculate and Print Average
// --------------------------------------------------------------------------
void calcAverage(void *pvParameters)
{
    uint16_t samples[ADC_BUF_SIZE];

    while (true)
    {
        // Wait for signal (from TimerTask OR SerialEchoTask)
        if (xSemaphoreTake(avg_sem, portMAX_DELAY) == pdTRUE)
        {
            uint8_t count_snapshot = 0;

            // Copy buffer to local array safely
            portENTER_CRITICAL(&adc_buf_mux);
            count_snapshot = adc_count;
            for (uint8_t i = 0; i < ADC_BUF_SIZE; i++)
            {
                samples[i] = adc_buf[i];
            }
            // Reset buffer count so we can start fresh
            adc_count = 0;
            portEXIT_CRITICAL(&adc_buf_mux);

            // Calculate Average
            if (count_snapshot > 0)
            {
                uint32_t sum = 0;
                // Only average the samples we actually collected
                uint8_t limit = (count_snapshot < ADC_BUF_SIZE) ? count_snapshot : ADC_BUF_SIZE;

                for (uint8_t i = 0; i < limit; i++)
                {
                    sum += samples[i];
                }
                uint16_t avg = sum / limit;

                Serial.print(">>> ADC Average (last ");
                Serial.print(limit);
                Serial.print(" samples): ");
                Serial.println(avg);
            }
            else
            {
                Serial.println(">>> ADC Average: No samples to average.");
            }
        }
    }
}

// --------------------------------------------------------------------------
// Task 3: Serial Echo & Command Handler
// --------------------------------------------------------------------------
#define CMD_BUF_SIZE 64

void serialEchoTask(void *pvParameters)
{
    char cmd_buf[CMD_BUF_SIZE];
    uint8_t idx = 0;

    while (1)
    {
        // Check if data is available on Serial
        while (Serial.available())
        {
            char c = Serial.read();

            if (c != '\r' && c != '\n')
            {
                // Echo back
                Serial.print(c);
            }

            // Handle Newline (End of command)
            if (c == '\r' || c == '\n')
            {
                if (idx > 0)
                {
                    cmd_buf[idx] = '\0'; // Null-terminate string

                    // Check for "avg" command
                    if (strcmp(cmd_buf, "avg") == 0)
                    {
                        Serial.println("Manual trigger: Calculating average...");
                        xSemaphoreGive(avg_sem);
                    }

                    idx = 0; // Reset buffer
                }
            }
            else if (idx < CMD_BUF_SIZE - 1)
            {
                // Add char to buffer
                cmd_buf[idx++] = c;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Yield to let other tasks run
    }
}

// --------------------------------------------------------------------------
// Main Setup
// --------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    vTaskDelay(pdMS_TO_TICKS(1000));

    Serial.println("--- ESP32 Timer + ADC + Tasks Demo ---");

    // 1. Create Semaphores
    timerSem = xSemaphoreCreateBinary();
    avg_sem = xSemaphoreCreateBinary();

    if (timerSem == nullptr || avg_sem == nullptr)
    {
        Serial.println("Error: Could not create semaphores");
        while (1)
            vTaskDelay(1000);
    }

    // 2. Create Tasks
    xTaskCreatePinnedToCore(timerTask, "Timer Task", 4096, NULL, 1, NULL, app_cpu);
    xTaskCreatePinnedToCore(calcAverage, "Avg Task", 4096, NULL, 1, NULL, app_cpu);
    xTaskCreatePinnedToCore(serialEchoTask, "Echo Task", 4096, NULL, 1, NULL, app_cpu);

    timer = timerBegin(timer_frequency_hz);

    if (timer == nullptr)
    {
        Serial.println("Error: Could not init timer");
        while (1)
            vTaskDelay(1000);
    }

    // Attach onTimer function to hardware timer
    timerAttachInterrupt(timer, &onTimer);

    // Enable Timer to work in autoreload mode
    timerAlarm(timer, timer_max_count, true, 0);
}

void loop()
{
    // Empty. All logic is in FreeRTOS tasks.
}
