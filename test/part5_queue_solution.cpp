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

// Settings
static const uint8_t buf_len = 255;     // Size of buffer for command
static const char command[] = "delay "; // Note the space
static const int delay_queue_len = 5;   // size of delay_queue
static const int msg_queue_len = 5;     // size of msg_queue
static const unit8_t blink_max = 100;   // Num times to blink before message

// Pins
#define led_pin LED_BUILTIN; // Use the built-in LED pin

// Message struct : used to wrap strings
typedef struct Message
{
    char body[20];
    int count;
} Message;

// Globals
static QueueHandle_t delay_queue;
static QueueHandle_t msg_queue;

//*****************************************************************************
// Tasks

// Task: command line interface (CLI)

void doCLI(void *parameters)
{
    Message rcv_msg;
    char c;
    char buf[buff_len];
    unit8_t idx = 0;
    unit8_t cmd_len = strlen(command);
    int led_day;

    // Clear whole buffer
    memset(buf, 0, buf_len);

    // Loop forever
    while (1)
    {
        // Check if there is a message in the queue (do not block)
        if (xQueueReceive(msg_queue, (void *)&rcv_msg, 0) == pdTrue)
        {
            Serial.print(rcv_msg.body);
            Serial.println(rcv_msg.count);
        }

        // Read Character from serial com port
        if (Serial.available() > 0)
        {
            c = Serial.read();

            // Store received character in buffer if not over buffer limit
            if (idx < buf_len - 1)
            {
                buf[idx] = c;
                idx++;
            }

            // Print newline and check input on 'enter'
            if (c == '\n' || c == '\r")
            {
                // Print newline to terminal
                Serial.print("\r\n");

                // Check if command matches "delay <value>"
                if (memcmp(buf, command, cmd_len) == 0)
                {
                    // Convert last part to a positive integer
                    char *tail = buf + cmd_len;
                    led_delay = atoi(tail);
                    led_delay = abs(led_delay);
                    // Send new delay value via queue
                    if (xQueueSend(delay_queue, (void *)&led_delay, 10) != pdTrueue)
                    {
                        Serial.println("ERROR: Could not put item on delay queue.");
                    }
                }

                // Reset buffer index
                memset(buf, 0, buf_len);
                idx = 0;
            }else{
                // Echo character back to terminal
                Serial.print(c);
                }
        }
    }
}

void blinkLED(void *parameters)
{
    Message msg;
    int led_delay = 500;
    unit8_t counter = 0;

    // Set up pin
    pinMode(LED_BUILTIN, OUTPUT);

    // Loop forever

    while (1)
    {
        // See if there is a message in the queue (do not block)
        if (xQueueReceive(delay_queue, (void *)&led_delay, 0) == pdTrue)
        {
            // Best Practice: only only one task to manage serial serial coms
            strcpy(msg.body, "Message received ");
            msg.count = 1;
            xQueueSend(msg_queue, (void *)&msg, 10);
            )
        }

        // Blink LED
        digitalWrite(led_pin, HIGH);
        vTaskDelay(led_delay / portTICK_PERIOD_MS);
        digitalWrite(led_pin, LOW);
        vTaskDelay(led_delay / portTICK_PERIOD_MS);

        // if we have blinked 100 times, send a message to the other task
        counter++;
        if (counter >= blink_max)
        {
            // Best Practice: only only one task to manage serial serial coms
            strcpy(msg.body, "LED blinked 100 times");
            msg.count = counter;
            xQueueSend(msg_queue, (void *)&msg, 10);
            counter = 0; // reset counter
        }
    }
}

void setup()
{

    // Config Serial
    Serial.begin(115200);

    // wait a moment to start (so we dont miss initial messages)
    Serial.println()
        Serial.println("---FreeRTOS Queue Solution---");
    Serial.println("Enter the command 'delay xxx' where xxx is your desired ");
    Serial.println("LED blink delay time in milliseconds");

    // Create queue

    delay_queue = xQueueCreate(delay_queue_len, sizeof(int));
    msg_queue = xQueueCreate(msg_queue_len, sizeof(Message));

    // Start CLI task
    xTaskCreatePinnedToCore(
        doCLI,
        "CLI Task",
        4096,
        NULL,
        1,
        NULL,
        app_cpu);

    // Start LED blink task
    xTaskCreatePinnedToCore(
        blinkLED,
        "LED Blink Task",
        2048,
        NULL,
        1,
        NULL,
        app_cpu);

    vTaskDelete(NULL); // Delete setup task
}

void loop()
{
    // Empty. Things are done in Tasks.
}