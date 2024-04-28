#pragma once
#include <Arduino.h>
#include "typedefs.h"

// implement class Pump. It takes pin and isInverted. It must on and off pin state. It has feature: off after time in ms. Class uses events to conroll pump state. Timer uses hw_timer_t from Arduino esp32 library.


class Pump
{
public:
  enum class State
  {
    OFF,
    ON
  };
  struct pump_settings_t {
    int idle_time;
    float liters_per_minute;
    int max_off_time_ms;
  };

  Pump(int pin, bool isInverted, const char* fileName = nullptr) : pin_(pin), isInverted_(isInverted), fileName_(fileName) {
    // if (fileName_ != nullptr)
    //   loadConfig();
  }

  void on() { setState(State::ON); }
  void off() { setState(State::OFF); }
  void setState(State state) {
    // if (state_ == state)
    //   return;
    state_ = state;
    if (state_ == State::ON)
      turnOn();
    else
      turnOff();
  }
  
  Pump::State getState() { return state_; }
  pump_settings_t getPumpSettings() { return pump_settings_; }

  void addTime(int time_ms) {
    // if timer_ started recalculate time
    if (timer_ != nullptr) {
      uint64_t t = timerReadMilis(timer_);
      offTime_ += time_ms - t;
      // Serial.printf("t %d; offTime %d\n", t, offTime_);
      startTimer();
    } else {
      offTime_ = time_ms + pump_settings_.idle_time;
      on();
    }
    Serial.printf("NEW Time: %d\n", offTime_);
    // offTime_ = time_ms;
  }
  void addCapacityLiters(float capacity) {
    int time_ms = capacity / pump_settings_.liters_per_minute * 60000;
    Serial.printf("Adding capacity: %f time_ms: %d\n", capacity, time_ms);
    addTime(time_ms);
  }

  void loadConfig() {
    cJSON* json = nullptr;
    JsonConfig::load(fileName_, &json);
    if (json == nullptr) {
      Serial.println("Can't load pump config file");
      saveConfig();
      return;
    }
    deserializeSettings(json);
    cJSON_Delete(json);
  }

  void saveConfig() {
    cJSON* json = cJSON_CreateObject();
    serializeSettings(json);
    JsonConfig::save(fileName_, json);
    cJSON_Delete(json);
  }

  void printPumpSettings() {
    Serial.printf("Pump settings: idle_time: %d ms; liters_per_minute: %.3f l/min\n", pump_settings_.idle_time, pump_settings_.liters_per_minute);
    Serial.printf("Pump settings: max_off_time_ms: %d ms\n", pump_settings_.max_off_time_ms);
  }

  void setPumpConfig(cJSON* json) {
    deserializeSettings(json);
    saveConfig();
  }

private:
  void turnOn() {
    digitalWrite(pin_, isInverted_ ? LOW : HIGH);
    startTimer();
  }

  void turnOff() {
    digitalWrite(pin_, isInverted_ ? HIGH : LOW);
    stopTimer();
    offTime_ = 0;
  }

  static void staticTimerCallback(void* arg) {
    Pump* pump = static_cast<Pump*>(arg);
    pump->off();
  }

  void startTimer() {
    if (offTime_ == 0)
      return;
    stopTimer();
    // timerSemaphore_ = xSemaphoreCreateBinary();
    timer_ = timerBegin(1000000);
    timerAttachInterruptArg(timer_, staticTimerCallback, this);
    timerAlarm(timer_, offTime_ * 1000, false, 0);
    Serial.printf("startTimer < %d >\n", offTime_);
  }

  void stopTimer() {
    if (timer_ != nullptr) {
      timerEnd(timer_);
      timer_ = nullptr;
    }
  }

  void serializeSettings(cJSON* json) const {
    cJSON_AddNumberToObject(json, "idle_time", pump_settings_.idle_time);
    cJSON_AddNumberToObject(json, "liters_per_minute", pump_settings_.liters_per_minute);
    cJSON_AddNumberToObject(json, "max_off_time_ms", pump_settings_.max_off_time_ms);
  }

  void deserializeSettings(cJSON* json) {
    pump_settings_.idle_time = JSON_OBJECT_NOT_NULL(json, "idle_time", pump_settings_.idle_time);
    pump_settings_.liters_per_minute = JSON_OBJECT_NOT_NULL(json, "liters_per_minute", pump_settings_.liters_per_minute);
    pump_settings_.max_off_time_ms = JSON_OBJECT_NOT_NULL(json, "max_off_time_ms", pump_settings_.max_off_time_ms);
  }

  State state_ = State::OFF;
  int pin_;
  bool isInverted_;
  const char* fileName_;
  int offTime_ = 0;
  hw_timer_t* timer_ = nullptr;

  pump_settings_t pump_settings_ = { 5000, 10.0f, 10*60000 };
};

Pump* pump = new Pump(LED_BUILTIN, false, "/spiffs/pump_config.json");
