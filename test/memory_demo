#include <Arduino.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

void testTask(void *parameter)
{
    while (1)
    {
        int a = 1;
        int b[100];

        // Do something with array so it's not optimized out by the compiler
        for (int i = 0; i < 100; i++)
        {
            b[i] = a + 1;
        }
        Serial.println(b[0]);

        // Print out remaining stack memory (words)
        Serial.print("High water mark (words): ");
        Serial.println(uxTaskGetStackHighWaterMark(NULL));
    }
}

void setup()
{
    // Configrue Serial
    Serial.begin(115200);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("Starting memory demo test task...");
    xTaskCreatePinnedToCore(
        testTask,
        "Test Task",
        1500,
        NULL,
        1,
        NULL,
        app_cpu);
    vTaskDelete(NULL);
}

void loop()
{
    // Empty loop
}