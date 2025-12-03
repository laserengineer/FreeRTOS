#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include <BoardConfig.h>

#define led_pin LED_BUILTIN // Use the built-in LED pin

// ---- Configuration ----
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Settings
static const uint8_t buf_len = 255;     // Size of buffer for command input
static const char command[] = "delay "; // Command prefix (note the space)
static const int delay_queue_len = 5;   // Size of delay_queue
static const int msg_queue_len = 5;     // Size of msg_queue
static const uint8_t blink_max = 100;   // Number of blinks before sending a message

// Message struct: used to wrap strings for inter-task communication
typedef struct Message
{
    char body[100]; // Message text
    int count;      // Optional: count or other data
} Message;

// Globals: Queues for inter-task communication
static QueueHandle_t delay_queue;
static QueueHandle_t msg_queue;

//*****************************************************************************
// Task: Command Line Interface (CLI)
// Reads user input from Serial, parses commands, and sends new delay values to the blink task

void doCLI(void *parameters)
{
    Message rcv_msg;                   // Buffer for received messages from blink task
    char c;                            // Latest character read from Serial
    char buf[buf_len];                 // Input buffer for command line
    uint8_t idx = 0;                   // Current position in input buffer
    uint8_t cmd_len = strlen(command); // Length of the command prefix
    int led_delay;                     // Parsed delay value

    memset(buf, 0, buf_len); // Clear the input buffer

    while (1)
    {
        // Check if there is a message in the queue from the blink task (non-blocking)
        if (xQueueReceive(msg_queue, (void *)&rcv_msg, 0) == pdTRUE)
        {
            Serial.println(rcv_msg.body);
            // Optionally print rcv_msg.count if needed
        }

        // Read character from serial port if available
        while (Serial.available() > 0)
        {
            c = Serial.read();

            // Handle Enter (CR, LF, or CRLF)
            if (c == '\r' || c == '\n')
            {
                // If CR is followed by LF, skip the LF
                if (c == '\r' && Serial.peek() == '\n')
                {
                    Serial.read(); // consume the '\n'
                }

                Serial.println(); // Move to new line after Enter

                buf[idx] = '\0'; // Null-terminate the buffer

                // Only process if something was entered
                if (idx > 0)
                {
                    // Check if input starts with "delay "
                    if (memcmp(buf, command, cmd_len) == 0)
                    {
                        char *tail = buf + cmd_len;
                        Serial.println(tail); // Echo the delay value
                        led_delay = atoi(tail);
                        led_delay = abs(led_delay);

                        if (xQueueSend(delay_queue, (void *)&led_delay, 10) != pdTRUE)
                        {
                            Serial.println("ERROR: Could not put new delay settings on delay queue.");
                        }
                        else
                        {
                            Serial.print("MESSAGE: New LED delay set to ");
                            Serial.print(led_delay);
                            Serial.println(" ms");
                        }
                    }
                    else
                    {
                        Serial.println("ERROR: Unknown command.");
                        // (Optional: print ASCII codes for debugging)
                    }
                }

                // Reset buffer for next command
                memset(buf, 0, buf_len);
                idx = 0;
            }
            else if (idx < buf_len - 1)
            {
                Serial.print(c); // Echo character
                buf[idx++] = c;
            }
        }
    }
}

//*****************************************************************************
// Task: Blink LED
// Blinks the LED at the current delay interval, receives new delay values from CLI task

void blinkLED(void *parameters)
{
    Message msg;         // Message to send to CLI task
    int led_delay = 500; // Default blink delay in ms
    uint8_t counter = 0; // Blink counter

    pinMode(LED_BUILTIN, OUTPUT); // Set up LED pin

    while (1)
    {
        // Check for new delay value from CLI task (non-blocking)
        if (xQueueReceive(delay_queue, (void *)&led_delay, 0) == pdTRUE)
        {
            // Prepare and send a message to CLI task about the new delay
            sprintf(msg.body, "MESSAGE: New delay setting received %d", led_delay);
            msg.count = 1;
            xQueueSend(msg_queue, (void *)&msg, 10);
        }

        // Blink the LED
        digitalWrite(led_pin, HIGH);
        vTaskDelay(led_delay / portTICK_PERIOD_MS);
        digitalWrite(led_pin, LOW);
        vTaskDelay(led_delay / portTICK_PERIOD_MS);
        counter++;

        // After blink_max blinks, send a message to CLI task
        if (counter >= blink_max)
        {
            sprintf(msg.body, "MESSAGE: LED blinked %d times", blink_max);
            msg.count = counter;
            xQueueSend(msg_queue, (void *)&msg, 10);
            counter = 0; // Reset counter
        }
    }
}

//*****************************************************************************
// Arduino Setup: Initializes serial, queues, and tasks

void setup()
{
    Serial.begin(115200);

    // Print instructions to user
    Serial.println("---FreeRTOS Queue Solution---");
    Serial.println("Enter the command 'delay xxx' where xxx is your desired ");
    Serial.println("LED blink delay time in milliseconds");

    // Create queues for inter-task communication
    delay_queue = xQueueCreate(delay_queue_len, sizeof(int));
    msg_queue = xQueueCreate(msg_queue_len, sizeof(Message));

    // Start CLI task (handles user input)
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

    vTaskDelete(NULL); // Delete setup task (not needed after setup)
}

//*****************************************************************************
// Arduino Loop: Not used, as everything is handled by FreeRTOS tasks

void loop()
{
    // Empty. All work is done in tasks.
}
