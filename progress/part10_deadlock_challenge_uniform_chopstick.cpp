/**
 * ESP32 Dining Philosophers
 *
 * The classic "Dining Philosophers" problem in FreeRTOS form.
 *
 * Based on http://www.cs.virginia.edu/luther/COA2/S2019/pa05-dp.html
 *
 * Assuming a uniform chopstick semaphore to avoid deadlock.
 */

#include <Arduino.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Settings
enum
{
    NUM_TASKS = 5
}; // Number of tasks (philosophers)

enum
{
    NUM_CHOPSTICKS = 5
}; // Number of chopsticks

enum
{
    TASK_STACK_SIZE = 2048
}; // Bytes in ESP32, words in vanilla FreeRTOS

// Globals
static SemaphoreHandle_t bin_sem;       // Wait for parameters (Philosophers No.) to be read from setup to task
static SemaphoreHandle_t done_sem;      // Notifies main task when done
static SemaphoreHandle_t chopstick_sem; // Semaphore for chopsticks;

//*****************************************************************************
// Tasks

// The only task: eating
void eat(void *parameters)
{

    int num;
    char buf[50];

    // Copy parameter and increment semaphore count
    num = *(int *)parameters;
    xSemaphoreGive(bin_sem);

    xSemaphoreTake(chopstick_sem, portMAX_DELAY);
    xSemaphoreTake(chopstick_sem, portMAX_DELAY);
    sprintf(buf, "Philosopher %i took chopstick 1", num);
    Serial.println(buf);
    sprintf(buf, "Philosopher %i took chopstick 2", num);
    Serial.println(buf);

    // Do some eating
    sprintf(buf, "Philosopher %i is eating", num);
    Serial.println(buf);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // Put down higher-numbered chopstick
    xSemaphoreGive(chopstick_sem);
    xSemaphoreGive(chopstick_sem);
    sprintf(buf, "Philosopher %i returned chopstick 1", num);
    Serial.println(buf);

    // Put down lower-numbered chopstick
    // xSemaphoreGive(chopstick_sem);
    sprintf(buf, "Philosopher %i returned chopstick 2", num);
    Serial.println(buf);

    // Notify main task and delete self
    xSemaphoreGive(done_sem);
    vTaskDelete(NULL);
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup()
{

    char task_name[20];

    // Configure Serial
    Serial.begin(115200);

    // Wait a moment to start (so we don't miss Serial output)
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Dining Philosophers Hierarchy Solution---");

    // Create kernel objects before starting tasks
    bin_sem = xSemaphoreCreateBinary();
    chopstick_sem = xSemaphoreCreateCounting(NUM_CHOPSTICKS, NUM_CHOPSTICKS);
    done_sem = xSemaphoreCreateCounting(NUM_TASKS, 0);

    for (int i = 0; i < NUM_TASKS; i++)
    {
        sprintf(task_name, "Philosopher %i", i);
        xTaskCreatePinnedToCore(eat,
                                task_name,
                                TASK_STACK_SIZE,
                                (void *)&i,
                                1,
                                NULL,
                                app_cpu);
        xSemaphoreTake(bin_sem, portMAX_DELAY);
    }

    // Wait until all the philosophers are done
    for (int i = 0; i < NUM_TASKS; i++)
    {
        xSemaphoreTake(done_sem, portMAX_DELAY);
    }

    // Say that we made it through without deadlock
    Serial.println("Done! No deadlock occurred!");
}

void loop()
{
    // Do nothing in this task
}