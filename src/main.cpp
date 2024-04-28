#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#include "typedefs.h"
#include "radio.h"
#include "Pump.h"
#include "HCS301.h"
#include "FFat.h"

HCS301* hcs301 = new HCS301(0x001C4A01);

void initSPIFFS() {
  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false
  };
  esp_err_t ret = esp_vfs_spiffs_register(&conf);
  if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
          Serial.printf("[!!!] Failed to mount or format filesystem\n");
      } else if (ret == ESP_ERR_NOT_FOUND) {
          Serial.println("[!!!] Failed to find SPIFFS partition");
      } else {
          Serial.printf("[!!!] Failed to initialize SPIFFS (%s)\n", esp_err_to_name(ret));
      }
    return;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK) {
    Serial.printf("Failed to get SPIFFS partition information (%s)\n", esp_err_to_name(ret));
  } else {
    Serial.printf("Partition size: total: %d, used: %d\n", total, used);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  int count = 20;
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

  initSPIFFS();

  if (!initRadio()) {
    Serial.println("CC1101 initialization failed!");
    return;
  }
  Serial.println("CC1101 initialization successful!");
  
  pump->loadConfig();
  pump->printPumpSettings();

  hcs301->set_on_buttons_press([](EventBits_t button) {
    Serial.printf("Pressed button: %d\n", button);
    switch (button) {
      case 4:
        pump->off();
        break;
      case 6:
        pump->off();
        pump->addTime(pump->getPumpSettings().max_off_time_ms);
        break;
      case 9:
        if (pump->getState() == Pump::State::OFF) {
          pump->addCapacityLiters(5*4);
        } else {
          pump->addCapacityLiters(5);
        }
        break;
    }
  });
  start_wifi();
  start_webserver();

}

void loop() {
  vTaskDelay(10000 / portTICK_PERIOD_MS);
}