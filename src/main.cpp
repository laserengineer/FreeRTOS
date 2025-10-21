#include <Arduino.h>

int count1 = 0;
int count2 = 0;
#define LED_BUILTIN 2

void task1(void *parameters)
{
    for (;;)
    {                                    // infinite loop
        digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
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
        vTaskDelay(500 / portTICK_PERIOD_MS);
        digitalWrite(LED_BUILTIN, LOW);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
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
        NULL      // Task handle
    );
}

void loop() {}