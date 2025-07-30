#include <Arduino.h>

void setup() {
  // No need to initialize the RGB LED
  Serial.begin(115200);
}

// the loop function runs over and over again forever

void loop() {
#ifdef RGB_BUILTIN
  neopixelWrite(RGB_BUILTIN,RGB_BRIGHTNESS,RGB_BRIGHTNESS,RGB_BRIGHTNESS); // White
  Serial.println("White");
  delay(1000);
  neopixelWrite(RGB_BUILTIN,0,RGB_BRIGHTNESS,RGB_BRIGHTNESS); // Cyan
  Serial.println("Cyan");
  delay(1000);
  neopixelWrite(RGB_BUILTIN,RGB_BRIGHTNESS,RGB_BRIGHTNESS,0); // Yellow
  Serial.println("Yellow");
  delay(1000);
  neopixelWrite(RGB_BUILTIN,RGB_BRIGHTNESS,0,0); // Red
  Serial.println("Red");
  delay(1000);
  neopixelWrite(RGB_BUILTIN,0,RGB_BRIGHTNESS,0); // Green
  Serial.println("Green");
  delay(1000);
  neopixelWrite(RGB_BUILTIN,0,0,RGB_BRIGHTNESS); // Blue
  Serial.println("Blue");
  delay(1000);
  neopixelWrite(RGB_BUILTIN,0,0,0); // Off / black
  Serial.println("Off");
  delay(1000);
#endif
}