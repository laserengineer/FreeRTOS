// ISR Interrupts Service Routine (ISR) example using ESP32 hardware timer

#include <Arduino.h>
#include <BoardConfig.h>

// Settings
static const uint16_t timer_divider = 80;
static const uint64_t timer_max_count = 2000000;

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Globals
static hw_timer_t *timer = NULL;
volatile bool ledstate = false;
// #define LED_PIN LED_BUILTIN // Use the built-in LED pin

//*****************************************************************************
// Interrupt Service Routines (ISRs)

void IRAM_ATTR onTimer()
{
    ledstate = !ledstate;
    digitalWrite(LED_BUILTIN, ledstate ? HIGH : LOW);
    Serial.println(ledstate);
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup()
{
    Serial.begin(115200);
    // Wait a moment to start (so we don't miss Serial output)
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---CPU Timer---");
    setCpuFrequencyMhz(80);
    Serial.println("CPU");                // In MHz
    Serial.println(getCpuFrequencyMhz()); // In MHz

    pinMode(LED_BUILTIN, OUTPUT);

    // Create and start timer (num, divider, countUp)
    timer = timerBegin(1000000);
    // Provide ISR to timer (timer, function, edge)
    timerAttachInterrupt(timer, &onTimer);
    // At what count should ISR trigger (timer, count, autoreload)
    // timerWrite(timer, timer_max_count);

    // Allow ISR to trigger
    timerAlarm(timer, timer_max_count, true, 0);
}
void loop()
{
}