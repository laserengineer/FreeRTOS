#include <Arduino.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Settings
static const size_t msg_queue_len = 5;

// Global queue handle
static QueueHandle_t led_delay_queue = NULL;

//*****************************************************************************
// Tasks A 
// Print 
// Print messages received from the queue

void printQueueMessages(void *pvParameters)
{
    int item;
    while (1)
    {
        // Block for up to 500ms for a message
        if (xQueueReceive(msg_queue, &item, 10 / portTICK_PERIOD_MS) == pdTRUE)
        {
            Serial.print("Received from queue: ");
            Serial.println(item);
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void sendQueueMessages(void *pvParameters)
{
    int num = 12;
    while (1)
    {
        if (xQueueSend(msg_queue, &num, 0) == pdTRUE)
        {
            Serial.printf("Sent item %d to queue\n", num);
        }
        else
        {
            Serial.printf("Queue is full, item dropped: %d\n", num);
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
        num++;
    }
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Optional: Wait for serial monitor to connect
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Create queue and check for success
    msg_queue = xQueueCreate(msg_queue_len, sizeof(int));
    if (msg_queue == NULL)
    {
        Serial.println("Error creating the queue!");
        while (1)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    xTaskCreatePinnedToCore(
        sendQueueMessages,
        "Send Queue Messages",
        2048,
        NULL,
        1,
        NULL,
        app_cpu);

    xTaskCreatePinnedToCore(
        printQueueMessages,
        "Print Queue Messages",
        2048,
        NULL,
        1,
        NULL,
        app_cpu);
}

void loop()
{
    // Nothing to do here, tasks handle everything!
}
