#include <Arduino.h>
// #include <BoardConfig.h>

// Settings
static const uint16_t timer_divider = 80;
static const uint64_t timer_max_count = 1000000;

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#define LED_PIN LED_BUILTIN // Use the built-in LED pin

// Globals
static hw_timer_t *timer = NULL;
bool ledstate = false;

//*****************************************************************************
// Interrupt Service Routines (ISRs)

void IRAM_ATTR onTimer()
{
    ledstate = !ledstate;
    rgbLedWrite(RGB_BUILTIN, ledstate ? RGB_BRIGHTNESS : 0, ledstate ? RGB_BRIGHTNESS : 0, ledstate ? RGB_BRIGHTNESS : 0);
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

    // Create and start timer (num, divider, countUp)
    timer = timerBegin(0, timer_divider, true);
    // Provide ISR to timer (timer, function, edge)
    timerAttachInterrupt(timer, &onTimer, true);
    // At what count should ISR trigger (timer, count, autoreload)
    timerAlarmWrite(timer, timer_max_count, true);

    // Allow ISR to trigger
    timerAlarmEnable(timer);
}
void loop()
{
}