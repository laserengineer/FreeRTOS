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

        // Print out number of free heap memory bytes before malloc
        Serial.print("Heap before malloc (bytes): ");
        Serial.println(xPortGetFreeHeapSize());

        int *ptr = (int *)pvPortMalloc(40960 * sizeof(int));

        // Check if memory allocation was successful
        if (ptr == NULL)
        {
            Serial.println("Memory allocation failed");
            return;
        }
        else
        { // Do something with allocated memory so it's not optimized out by the compiler
            for (int i = 0; i < 40960; i++)
            {
                ptr[i] = i;
            }

            Serial.print("Heap after malloc (bytes): ");
            Serial.println(xPortGetFreeHeapSize());
            vPortFree(ptr);
            Serial.print("Heap after free (bytes): ");
            Serial.println(xPortGetFreeHeapSize());
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
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