// Replace the manual buffer and counting semaphore with queues

#include <Arduino.h>
#include <stdint.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

static const uint8_t BUF_SIZE = 15;
static const int num_prod_tasks = 5;
static const int num_cons_tasks = 2;
static const int num_writes = 3;

static SemaphoreHandle_t bin_sem;
static SemaphoreHandle_t mutex;
static QueueHandle_t buffer_queue;

void producer(void *parameters)
{
    int num = (int)(intptr_t)parameters; // Correct: pass value, not pointer
    xSemaphoreGive(bin_sem);

    for (int i = 0; i < num_writes; i++)
    {
        xSemaphoreTake(mutex, portMAX_DELAY);
        xQueueSend(buffer_queue, (void *)&num, portMAX_DELAY);
        xSemaphoreGive(mutex);
    }
    vTaskDelete(NULL);
}

void consumer(void *parameters)
{
    int val;
    while (1)
    {

        if (xQueueReceive(buffer_queue, (void *)&val, portMAX_DELAY) == pdTRUE)
        {
            Serial.println(val);
        }
    }
}

void setup()
{
    char task_name[12];
    Serial.begin(115200);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Semaphore Queue Solution---");

    bin_sem = xSemaphoreCreateBinary();
    mutex = xSemaphoreCreateMutex();
    buffer_queue = xQueueCreate(BUF_SIZE, sizeof(int));

    for (int i = 0; i < num_prod_tasks; i++)
    {
        sprintf(task_name, "Producer %i", i);
        xTaskCreatePinnedToCore(
            producer,
            task_name,
            1024,
            (void *)(intptr_t)i, // Pass value directly
            1,
            NULL,
            app_cpu);
        xSemaphoreTake(bin_sem, portMAX_DELAY);
    }

    for (int i = 0; i < num_cons_tasks; i++)
    {
        sprintf(task_name, "Consumer %i", i);
        xTaskCreatePinnedToCore(
            consumer,
            task_name,
            1024,
            NULL,
            1,
            NULL,
            app_cpu);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("All tasks created");
}

void loop()
{
}
