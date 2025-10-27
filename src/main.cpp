#include <Arduino.h>
#include <StringUtils.h>
#include "BoardConfig.h"

volatile int ledDelay = 500; // Initial delay in milliseconds

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
                            Serial.print("\nLED delay updated to: ");
                            Serial.println(ledDelay);
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
        NULL);
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
