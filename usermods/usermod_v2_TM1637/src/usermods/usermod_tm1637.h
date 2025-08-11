
// src/usermods/usermod_tm1637.h
#pragma once
#include "wled.h"
#include "TM1637Display.h"

#ifndef USERMOD_ID_TM1637
  #define USERMOD_ID_TM1637  0x0142
#endif

class UsermodTM1637 : public Usermod {
 public:
  void setup() override;
  void loop() override;
  void addToConfig(JsonObject& root) override;
  bool readFromConfig(JsonObject& root) override;
  void addToJsonInfo(JsonObject& root) override;
  uint16_t getId() override { return USERMOD_ID_TM1637; }
  inline const char* getName() { return "TM1637 Display"; }

 private:
  int pinClk = 21;
  int pinDio = 22;
  int displayBrightness = 4;
  int pulseMax = 7;
  int pulsePeriod = 2000;
  bool showClock = true;

  TM1637Display* display = nullptr;
  bool initialized = false;
  unsigned long lastUpdate = 0;
  const uint16_t updateInterval = 200;
  const unsigned long initDelay = 2000;

  void ensureInit();
  static int clampToRange(int v, int lo, int hi);
  static bool isBadGpio(int pin);
};
