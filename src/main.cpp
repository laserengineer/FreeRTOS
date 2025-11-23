#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include <BoardConfig.h>

// ---- Configuration ----
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#define LED_PIN LED_BUILTIN        // Use the built-in LED pin
#define SERIAL_BUF_SIZE 64         // Size of the serial input buffer
#define LED_QUEUE_LEN 1            // Only one LED delay value needed at a time
#define LED_BLINK_MSG_QUEUE_LEN 64 // Buffer size for blink messages
#define BLINK_REPORT_INTERVAL 100  // How often to report blink count

// Queuue for inter-task communication

static QueueHandle_t led_delay_queue = NULL; // Queue for passing new LED delay values
static QueueHandle_t led_blink_queue = NULL; // Queue for passing blink count messages

// ---- Serial Monitor Task ----
// This task handles serial input and ouput, including echoing commands and printing blink reports
void serialMonitorTask(void *pvParameters)
{
    char buf[SERIAL_BUF_SIZE];               // Buffer for incoming serial data
    size_t len = 0;                          // Current length of data in buffer
    char msg_queue[LED_BLINK_MSG_QUEUE_LEN]; // Buffer for messages from LED blink task

    while (1)
    {
        // Read incoming serial data one character at a time
        while (Serial.available())
        {
            char c = Serial.read();     // Read a character from serial buffer
            if (c == '\n' || c == '\r') // If end of line detected
            {
                buf[len] = '\0'; // Null-terminate the string
                if (len > 0)
                {
                    Serial.println(buf);

                    // Parse command: look for "delay <value>" using sscanf
                    int value;
                    if (sscanf(buf, "delay %d", &value) == 1 && value > 0)
                    {
                        // Send new delay value to the LED blink task
                        xQueueOverwrite(led_delay_queue, &value);
                        Serial.printf("LED delay interval set to %d ms\n", value);
                    }
                }

                len = 0; // Reset buffer
            }
            else if (len < SERIAL_BUF_SIZE - 1)
            {
                buf[len++] = c; // Store character in buffer
                Serial.print(c);
            }
        }

        // check if there is's a new blink message from the LED blink task
        if (xQueueReceive(led_blink_queue, msg_queue, 10) == pdTRUE)
        {
            Serial.printf("%s", msg_queue); // Print the blink message
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // Short delay to yield CPU
    }
}

// ---- LED Blink Task ----
// This task blinks the LED at intervals specified by the serial monitor task

void ledBlinkTask(void *pvParameters)
{
    int blink_interval = 500; // Default 500 ms
    pinMode(LED_PIN, OUTPUT);
    int new_interval;
    int led_blink_count = 0;
    char msg_buf[LED_BLINK_MSG_QUEUE_LEN];

    while (1)
    {
        // Check if a new delay interval has been received from the serial task

        if (xQueueReceive(led_delay_queue, &new_interval, 10) == pdTRUE)
        {
            blink_interval = new_interval; // Update blink interval
            led_blink_count = 0;           // Reset blink count
        }
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(blink_interval / portTICK_PERIOD_MS);
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(blink_interval / portTICK_PERIOD_MS);
        led_blink_count++;

        if (led_blink_count % BLINK_REPORT_INTERVAL == 0 && led_blink_count != 0)
        {
            // Print to serial for immediate feedback
            Serial.printf("LED blink: %d times\n", led_blink_count);

            // Format message and send to serial monitor task via queue
            snprintf(
                msg_buf, sizeof(msg_buf),
                "LED has blinked %d times\n", led_blink_count);

            // Try to send the message to the queue
            if (xQueueSend(led_blink_queue, msg_buf, 0) != pdPASS)
            {
                Serial.println("Warning: LED blink queue full, message dropped.");
            }
        }
    }
}

// ---- Setup & Loop ----
void setup()
{
    Serial.begin(115200);

    // Wait for serial port to be ready (with timeout to avoid infinite wait)
    unsigned long start = millis();
    while (!Serial && millis() - start < 5000)
    {
        vTaskDelay(10);
    }
    // Create queue for LED delay values (holds one int)
    led_delay_queue = xQueueCreate(LED_QUEUE_LEN, sizeof(int));
    if (!led_delay_queue)
    {
        Serial.println("Failed to create LED delay queue!");
        while (1)
        {
            vTaskDelay(1000);
        }
    }

    // Create queue for blink messages (holds one message buffer)
    led_blink_queue = xQueueCreate(LED_QUEUE_LEN, LED_BLINK_MSG_QUEUE_LEN);
    if (!led_blink_queue)
    {
        Serial.println("Failed to create LED blink message queue!");
        while (1)
        {
            vTaskDelay(1000);
        }
    }

    // Start the serial monitor and LED blink tasks, pinned to the selected CPU core

    xTaskCreatePinnedToCore(serialMonitorTask, "SerialMonitor", 2048, NULL, 1, NULL, app_cpu);
    xTaskCreatePinnedToCore(ledBlinkTask, "LEDBlink", 2048, NULL, 1, NULL, app_cpu);
}

void loop()
{
    // Not used
}
