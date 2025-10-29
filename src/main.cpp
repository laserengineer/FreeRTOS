#include <Arduino.h>
#include <StringUtils.h>
#include "BoardConfig.h"

volatile int ledDelay = 500; // Initial delay in milliseconds
// Handle to the LED toggle task so we can delete/recreate it at runtime
TaskHandle_t ledTaskHandle = NULL;

void ledToggleTask(void *pvParameters)
{
    pinMode(LED_BUILTIN, OUTPUT);
    bool ledState = false;
    while (1)
    {
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
        vTaskDelay(pdMS_TO_TICKS(ledDelay));
    }
}

// Delete the existing LED task (if any) and create a new one that will use
// the current value of `ledDelay`.
void restartLedTask()
{
    if (ledTaskHandle != NULL)
    {
        vTaskDelete(ledTaskHandle);
        ledTaskHandle = NULL;
        // brief delay to let the scheduler finish deleting the task
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    xTaskCreate(
        ledToggleTask,
        "LED Toggle Task",
        1024,
        NULL,
        1,
        &ledTaskHandle);
}

void serialInputTask(void *pvParameters)
{
    String inputString = "";
    char lastChar = 0;
    Serial.println("Enter LED delay in ms (positive integer):");
    while (1)
    {
        if (Serial.available())
        {
            char c = Serial.read();
            if (c != '\n' && c != '\r')
            {
                Serial.write(c);
            } // Echo back the received character

            // Only process on line ending, and ignore double-processing (CR+LF)
            if ((c == '\n' || c == '\r') && (lastChar != '\r' && c == '\n'))
            {
                // Do nothing, already handled by previous '\r'
            }
            else if (c == '\n' || c == '\r')
            {
                if (inputString.length() > 0)
                {
                    if (isValidPositiveInteger(inputString))
                    {
                        int newDelay = inputString.toInt();
                        if (newDelay > 0)
                        {
                            ledDelay = newDelay;
                            // recreate the LED task so it uses the (possibly) new delay value
                            restartLedTask();
                            Serial.printf("\nLED delay updated to: %d ms.\n", ledDelay);
                        }
                        else
                        {
                            Serial.println("Please enter a number greater than zero.");
                        }
                    }
                    else
                    {
                        Serial.println("Invalid input. Only positive numbers allowed.");
                    }
                    inputString = "";
                    Serial.println("Enter LED delay in ms (positive integer):");
                }
            }
            else
            {
                if (isDigit(c))
                {
                    inputString += c;
                }
            }
            lastChar = c;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    pinMode(LED_BUILTIN, OUTPUT);
    xTaskCreate(
        ledToggleTask,
        "LED Toggle Task",
        1024,
        NULL,
        1,
        &ledTaskHandle);
    xTaskCreate(
        serialInputTask,
        "Serial Input Task",
        2048,
        NULL,
        1,
        NULL);
}

void loop()
{
    // Nothing to do here, tasks handle everything!
}
