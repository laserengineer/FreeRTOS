#include <Arduino.h>
#include <FreeRTOS.h>

// Only use core 1
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Pins
static const int led_pin = 48;

void task1(void *parameters)
{
  while (1)
  {
    Serial.println("Task 1: Running on core ");
    // Serial.printlnln(xPortGetCoreID());
    digitalWrite(led_pin, HIGH);
    Serial.println("Task 1: Set LED HIGH on core ");
    vTaskDelay(500 / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    Serial.println("Task 1: Set LED low on core ");
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void task2(void *parameters)
{
  while (1)
  {
    Serial.println("Task 2: Running on core ");
    // Serial.printlnln(xPortGetCoreID());
    digitalWrite(led_pin, HIGH);
    Serial.println("Task 2: Set LED HIGH on core ");
    vTaskDelay(300 / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    Serial.println("Task 2: Set LED low on core ");
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting tasks...");
  pinMode(led_pin, OUTPUT);

  // Create two tasks
  // xTaskCreate is a FreeRTOS function to create a task
  // The task will run on the core specified by app_cpu
  // Each task will print its own counter value every second
  xTaskCreatePinnedToCore(
      task1,    // Function that should be called
      "Task 1", // Name of the task (for debugging)
      1000,     // Stack size (bytes)
      NULL,     // Parameter to pass
      1,        // Task priority
      NULL,     // Task handle
      app_cpu   // CPU handle
  );

  xTaskCreatePinnedToCore(
      task2,    // Function that should be called
      "Task 2", // Name of the task (for debugging)
      1000,     // Stack size (bytes)
      NULL,     // Parameter to pass
      1,        // Task priority
      NULL,     // Task handle
      app_cpu   // CPU handle

  );
}

void loop() {}
