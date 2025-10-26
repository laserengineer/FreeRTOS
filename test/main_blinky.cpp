#include <Arduino.h>

// Only use core 1 (ESP32)
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

static const int led_pin = 2; // Use built-in LED

void task1(void *parameters)
{
  for (;;)
  {
    digitalWrite(led_pin, HIGH);
    Serial.print("Task 1: Set LED HIGH, running on core ");
    Serial.println(xPortGetCoreID());
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void task2(void *parameters)
{
  for (;;)
  {
    Serial.print("Task 2: Set LED LOW, running on core ");
    Serial.println(xPortGetCoreID());
    vTaskDelay(500 / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(led_pin, OUTPUT);

  xTaskCreatePinnedToCore(
      task1, "Task 1", 1000, NULL, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(
      task2, "Task 2", 1000, NULL, 1, NULL, app_cpu);
}

void loop() {}
