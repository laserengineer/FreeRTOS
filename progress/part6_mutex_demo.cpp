#include semphr.h

// ---- Configuration ----
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Globals: Mutex for protecting Serial access
static int shared_var = 0; // Example shared variable
static semaphoreHandle_t mutex;
//*****************************************************************************

// Task: Mutex Demo
// Increment shared variable (the wrong way, without mutex protection)

void incTask(void *parameters)
{
    int local_var;

    // Loop forever
    while (1)
    {
        // Roundabout way to do "Shared_var++" ramdomly and poorly
        if (xSemaphoreTake(mutex, 0) == pdTrue)
        {
            local_var = shared_var;
            local_var++;
            vTaskDelay(random(100, 500) / portTICK_PERIOD_MS);
            shared_var = local_var;

            xSemaphoreGive(mutex);

            // Print the value of shared_var);
            Serial.println(shared_var);
        }
        else
        {
            // Could not obtain the mutex
            Serial.println("Could not obtain mutex");
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

    // Wait a moment to start

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("--- FreeRTOS Race Condition Demo ---");
    Serial.println("Incrementing a shared variable without mutex protection.");

    // Create mutex-protected section
    mutex = xSemaphoreCreateMutex();

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