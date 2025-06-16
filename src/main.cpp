#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for serial to initialize
  Serial.println("Hello, PlatformIO World!");
}

void loop() {
  Serial.println("Hello from loop!");
  delay(1000);
}
