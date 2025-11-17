#include <Arduino.h>
#include <string.h>
#include <stdio.h>

// ---- Configuration ----
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#define LED_PIN LED_BUILTIN
#define SERIAL_BUF_SIZE 64
#define LED_QUEUE_LEN 1

static QueueHandle_t led_delay_queue = NULL;

// ---- Serial Monitor Task ----
void serialMonitorTask(void *pvParameters)
{
    char buf[SERIAL_BUF_SIZE];
    size_t len = 0;

    while (1)
    {
        // Read bytes one by one
        while (Serial.available())
        {
            char c = Serial.read();
            if (c == '\n' || c == '\r')
            {
                buf[len] = '\0'; // Null-terminate
                if (len > 0)
                {
                    Serial.print("Echo: ");
                    Serial.println(buf);

                    // Parse command using sscanf
                    int value;
                    if (sscanf(buf, "delay %d", &value) == 1 && value > 0)
                    {
                        xQueueOverwrite(led_delay_queue, &value);
                        Serial.printf("LED delay interval set to %d ms\n", value);
                    }
                }
                len = 0; // Reset buffer
            }
            else if (len < SERIAL_BUF_SIZE - 1)
            {
                buf[len++] = c;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// ---- LED Blink Task ----
void ledBlinkTask(void *pvParameters)
{
    int blink_interval = 500; // Default 500 ms
    pinMode(LED_PIN, OUTPUT);

    while (1)
    {
        int new_interval;
        if (xQueueReceive(led_delay_queue, &new_interval, 0) == pdTRUE)
        {
            blink_interval = new_interval;
        }
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(blink_interval / portTICK_PERIOD_MS);
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(blink_interval / portTICK_PERIOD_MS);
    }
}

// ---- Setup & Loop ----
void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        vTaskDelay(10);
    }

    led_delay_queue = xQueueCreate(LED_QUEUE_LEN, sizeof(int));
    if (!led_delay_queue)
    {
        Serial.println("Failed to create LED delay queue!");
        while (1)
        {
            vTaskDelay(1000);
        }
    }

    xTaskCreatePinnedToCore(serialMonitorTask, "SerialMonitor", 2048, NULL, 1, NULL, app_cpu);
    xTaskCreatePinnedToCore(ledBlinkTask, "LEDBlink", 2048, NULL, 1, NULL, app_cpu);
}

void loop()
{
    // Not used
}
