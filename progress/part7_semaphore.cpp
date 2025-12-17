/**
 * FreeRTOS Counting Semaphore Challenge
 *
 * Challenge: use a mutex and counting semaphores to protect the shared buffer
 * so that each number (0 throguh 4) is printed exactly 3 times to the Serial
 * monitor (in any order). Do not use queues to do this!
 *
 * Hint: you will need 2 counting semaphores in addition to the mutex, one for
 * remembering number of filled slots in the buffer and another for
 * remembering the number of empty slots in the buffer.
 *
 * Date: January 24, 2021
 * Author: Shawn Hymel
 * License: 0BSD
 **/

#include <Arduino.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Settings
enum
{
    BUF_SIZE = 5
};
static const int num_prod_tasks = 5;
static const int num_cons_tasks = 2;
static const int num_writes = 3;

// Globals
static int buf[BUF_SIZE];
static int head = 0;
static int tail = 0;
static SemaphoreHandle_t bin_sem;
static SemaphoreHandle_t empty_slots;
static SemaphoreHandle_t filled_slots;
static SemaphoreHandle_t mutex;

//*****************************************************************************
// Producer: write a given number of times to shared buffer
void producer(void *parameters)
{
    int num = *(int *)parameters;
    free(parameters);

    // Signal to setup() that parameter has been read
    xSemaphoreGive(bin_sem);

    for (int i = 0; i < num_writes; i++)
    {
        xSemaphoreTake(empty_slots, portMAX_DELAY);
        xSemaphoreTake(mutex, portMAX_DELAY);
        buf[head] = num;
        head = (head + 1) % BUF_SIZE;
        xSemaphoreGive(mutex);
        xSemaphoreGive(filled_slots);
    }

    vTaskDelete(NULL);
}

// Consumer: continuously read from shared buffer
void consumer(void *parameters)
{
    int val;
    while (1)
    {
        xSemaphoreTake(filled_slots, portMAX_DELAY);
        xSemaphoreTake(mutex, portMAX_DELAY);
        val = buf[tail];
        tail = (tail + 1) % BUF_SIZE;
        xSemaphoreGive(mutex);
        xSemaphoreGive(empty_slots);

        // Print outside mutex for better performance
        Serial.println(val);
    }
}

//*****************************************************************************
// Main

void setup()
{
    char task_name[20];

    Serial.begin(115200);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Semaphore Refined Solution---");

    // Create mutexes and semaphores before starting tasks
    bin_sem = xSemaphoreCreateBinary();
    mutex = xSemaphoreCreateMutex();
    empty_slots = xSemaphoreCreateCounting(BUF_SIZE, BUF_SIZE);
    filled_slots = xSemaphoreCreateCounting(BUF_SIZE, 0);

    // Start producer tasks (wait for each to read argument)
    for (int i = 0; i < num_prod_tasks; i++)
    {
        int *arg = (int *)malloc(sizeof(int));
        *arg = i;
        sprintf(task_name, "Producer %d", i);
        xTaskCreatePinnedToCore(producer,
                                task_name,
                                1024,
                                arg,
                                1,
                                NULL,
                                app_cpu);
        xSemaphoreTake(bin_sem, portMAX_DELAY);
    }

    // Start consumer tasks
    for (int i = 0; i < num_cons_tasks; i++)
    {
        sprintf(task_name, "Consumer %d", i);
        xTaskCreatePinnedToCore(consumer,
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
