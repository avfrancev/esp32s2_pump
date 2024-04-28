#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
// #include "typedefs.h"

const char *ssid = "BEE_465CM"; // Change this to your WiFi SSID
const char *password = "123qwe123"; // Change this to your WiFi password

void start_wifi() {
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid, password);
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());
    }
  });
}