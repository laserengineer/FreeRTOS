#include <Arduino.h>

static const BaseType_t app_cpu = 1; // Use core 1 for tasks
static const size_t BUF_SIZE = 255;

// Create printMsg handle globally to be used by Task A for sending notifications
TaskHandle_t printMsghandle = NULL;
TaskHandle_t readSerialhandle = NULL;

//*****************************************************************************
// Tasks

void readSerial(void *pvParameters)
{
    char c;
    char buf[BUF_SIZE];
    size_t idx = 0;

    // Clear buffer
    memset(buf, 0, BUF_SIZE);

    while (1)
    {
        while (Serial.available())
        {
            c = Serial.read();

            if (idx < BUF_SIZE - 1)
            {
                buf[idx++] = c;
            }

            if (c == '\n')
            {
                // The last character is is '\n', so replace it with null terminator
                buf[idx - 1] = '\0';

                // Try to allocate heap memory for the message
                char *heapMsg = (char *)pvPortMalloc(idx * sizeof(char));
                // If malloc returns 0 (out of memory), throw an error and reset
                configASSERT(heapMsg);

                if (heapMsg)
                {
                    // copy including the null terminator (buf[idx-1] == '\0')
                    memcpy(heapMsg, buf, idx);
                    // Send pointer to the print task via notification value
                    xTaskNotify(printMsghandle, (uint32_t)heapMsg, eSetValueWithOverwrite);
                }

                // Reset buffer index for next message
                idx = 0;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void printMsg(void *pvParameters)
{
    uint32_t msg_ptr;
    while (1)
    {
        // Wait indefinitely for notification from readSerial
        if (xTaskNotifyWait(0, 0, &msg_ptr, portMAX_DELAY) == pdTRUE)
        {
            char *msg = (char *)msg_ptr;
            if (msg)
            {
                Serial.print("Received: ");
                Serial.println(msg);
                Serial.print("Free heap (bytes): ");
                Serial.println(xPortGetFreeHeapSize());

                vPortFree(msg);
                msg = NULL;
            }
        }
    }
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Start printMsg first so we have its handle
    xTaskCreatePinnedToCore(
        printMsg,
        "Print Message",
        2048,
        NULL,
        1,
        &printMsghandle,
        app_cpu);

    // Start readSerial
    xTaskCreatePinnedToCore(
        readSerial,
        "Read Serial",
        2048,
        NULL,
        1,
        &readSerialhandle,
        app_cpu);
}

void loop()
{
    // Nothing to do here, tasks handle everything!
}
