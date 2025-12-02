#include <Arduino.h>
#include <FreeRTOS.h>

int count1 = 0;
int count2 = 0;

void task1(void *parameters)
{
  for (;;)
  { // infinite loop
    Serial.print("Task 1 counter: ");
    Serial.println(count1++);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void task2(void *parameters)
{
  for (;;)
  { // infinite loop
    Serial.print("Task 2 counter: ");
    Serial.println(count2++);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting tasks...");

  // Create two tasks
  // xTaskCreate is a FreeRTOS function to create a task
  // The task will run on the core specified by app_cpu
  // Each task will print its own counter value every second
  xTaskCreate(
      task1,    // Function that should be called
      "Task 1", // Name of the task (for debugging)
      1000,     // Stack size (bytes)
      NULL,     // Parameter to pass
      1,        // Task priority
      NULL      // Task handle
  );

  xTaskCreate(
      task2,    // Function that should be called
      "Task 2", // Name of the task (for debugging)
      1000,     // Stack size (bytes)
      NULL,     // Parameter to pass
      1,        // Task priority
      NULL
      // Task handle
  );
}

void loop() {}
