#include <Arduino.h>
#include <stdint.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

enum
{
    BUF_SIZE = 5
};
static const int num_prod_tasks = 5;
static const int num_cons_tasks = 2;
static const int num_writes = 3;

static int buf[BUF_SIZE];
static int write_index = 0;
static int read_index = 0;
static SemaphoreHandle_t bin_sem;
static SemaphoreHandle_t empty_slots;
static SemaphoreHandle_t filled_slots;
static SemaphoreHandle_t mutex;

void producer(void *parameters)
{
    int num = (int)(intptr_t)parameters; // Correct: pass value, not pointer
    xSemaphoreGive(bin_sem);

    for (int i = 0; i < num_writes; i++)
    {
        xSemaphoreTake(empty_slots, portMAX_DELAY);
        xSemaphoreTake(mutex, portMAX_DELAY);
        buf[write_index] = num;
        write_index = (write_index + 1) % BUF_SIZE;
        xSemaphoreGive(mutex);
        xSemaphoreGive(filled_slots);
    }
    vTaskDelete(NULL);
}

void consumer(void *parameters)
{
    int val;
    while (1)
    {
        xSemaphoreTake(filled_slots, portMAX_DELAY);
        xSemaphoreTake(mutex, portMAX_DELAY);
        val = buf[read_index];
        read_index = (read_index + 1) % BUF_SIZE;
        Serial.println(val);
        xSemaphoreGive(mutex);
        xSemaphoreGive(empty_slots);
    }
}

void setup()
{
    char task_name[12];
    Serial.begin(115200);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Semaphore CORRECTED Solution---");

    bin_sem = xSemaphoreCreateBinary();
    mutex = xSemaphoreCreateMutex();
    empty_slots = xSemaphoreCreateCounting(BUF_SIZE, BUF_SIZE);
    filled_slots = xSemaphoreCreateCounting(BUF_SIZE, 0);

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

    Serial.println("All tasks created");
}

void loop()
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}
