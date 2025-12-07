#include <Arduino.h>

// ---- Configuration ----
// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Globals: Mutex for protecting Serial access
static int shared_var = 0; // Example shared variable
static SemaphoreHandle_t mutex = nullptr;

//*****************************************************************************

// Task: Mutex Demo
// Increment shared variable, with mutex protection
void incTask(void *parameters)
{
    while (1)
    {
        // Roundabout way to do "Shared_var++" ramdomly and poorly
        if (xSemaphoreTake(mutex, 0) == pdTRUE)
        {
            // Critical section: safely increase shared_var
            shared_var++;
            Serial.print(pcTaskGetName(NULL));
            Serial.print(" : ");
            Serial.println(shared_var);

            // Release the mutex
            xSemaphoreGive(mutex);

            //Simulate variable task timing
            vTaskDelay(random(100, 500) / portTICK_PERIOD_MS);

            // Print the value of shared_var);
            taskYIELD();
        }

    }
}

//*****************************************************************************
// Main

void setup()
{
    randomSeed(analogRead(0));
    Serial.begin(115200);

    // Create the mutex
    mutex = xSemaphoreCreateMutex();

    if (mutex == nullprt)
    {
        Serial.println("Failed to create mutex");
        while (1)
            ;
    }

    // Wait a moment to start

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    Serial.println();
    Serial.println("--- FreeRTOS Race Condition Demo ---");
    Serial.println("Incrementing a shared variable with mutex protection.");

    // Create mutex-protected section

    // Start the increment task
    xTaskCreatePinnedToCore(
        incTask,
        "IncTask 1",
        2048,
        NULL,
        1,
        NULL,
        app_cpu);
    xTaskCreatePinnedToCore(
        incTask,
        "IncTask 2",
        2048,
        NULL,
        1,
        NULL,
        app_cpu);
    vTaskDelete(NULL); // Delete setup task (not needed after setup)
}

void loop()
{
    // Not used, as everything is handled by FreeRTOS tasks
}