#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#include "typedefs.h"
#include "radio.h"


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  int count = 50;
  while (!Serial && --count > 0)
  {
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
  }
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  // Serial.onEvent(usbEventCallback);

  Serial.println("===============================");

  if (!initRadio()) {
    Serial.println("CC1101 initialization failed!");
    return;
  }
  Serial.println("CC1101 initialization successful!");
}

void loop() { }