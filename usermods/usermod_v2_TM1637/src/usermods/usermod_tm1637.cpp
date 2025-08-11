
// src/usermods/usermod_tm1637.cpp
#include "usermod_tm1637.h"
#include <math.h>

static const __FlashStringHelper* CFG_NODE()        { return F("TM1637 Display"); }
static const __FlashStringHelper* CFG_CLK()         { return F("CLK Pin (GPIO)"); }
static const __FlashStringHelper* CFG_DIO()         { return F("DIO Pin (GPIO)"); }
static const __FlashStringHelper* CFG_BRIGHT()      { return F("Basishelligkeit (0-7)"); }
static const __FlashStringHelper* CFG_PULSEMAX()    { return F("Max. Pulshelligkeit (0-7)"); }
static const __FlashStringHelper* CFG_PULSPER()     { return F("Pulsperiode (ms)"); }
static const __FlashStringHelper* CFG_SHOW()        { return F("Uhrzeit anzeigen (true/false)"); }

int UsermodTM1637::clampToRange(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}
bool UsermodTM1637::isBadGpio(int pin) {
  if (pin >= 6 && pin <= 11) return true;
  if (pin >= 34 && pin <= 39) return true;
  if (pin == 0 || pin == 2 || pin == 12 || pin == 15) return true;
  if (pin == 1 || pin == 3) return true;
  if (pin < 0) return true;
  return false;
}
void UsermodTM1637::ensureInit() {
  if (initialized) return;
  if (millis() < initDelay) return;
  if (isBadGpio(pinClk) || isBadGpio(pinDio)) return;
  display = new TM1637Display(pinClk, pinDio);
  if (!display) return;
  display->setBrightness(clampToRange(displayBrightness, 0, 7));
  display->clear();
  uint8_t colonMask = 0b01000000;
  display->showNumberDecEx(1010, colonMask, true);
  initialized = true;
}
void UsermodTM1637::setup() {}
void UsermodTM1637::loop() {
  #ifdef WLED_DISABLE_USERMODS
    return;
  #endif
  if (!showClock || !ntpEnabled) return;
  ensureInit();
  if (!initialized || !display) return;
  unsigned long nowMs = millis();
  if (nowMs - lastUpdate < updateInterval) return;
  lastUpdate = nowMs;
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  if (!timeinfo) return;
  int hours = timeinfo->tm_hour;
  int minutes = timeinfo->tm_min;
  int period = pulsePeriod <= 0 ? 2000 : pulsePeriod;
  float t = float(nowMs % period) / float(period);
  int pulse = int((sinf(t * 2.0f * (float)M_PI) + 1.0f) * (pulseMax / 2.0f));
  pulse = clampToRange(pulse, 0, 7);
  int currentBrightness = displayBrightness;
  if (pulse > currentBrightness) currentBrightness = pulse;
  currentBrightness = clampToRange(currentBrightness, 0, 7);
  display->setBrightness(currentBrightness);
  const uint8_t colonMask = 0b01000000;
  int value = (hours % 100) * 100 + (minutes % 100);
  display->showNumberDecEx(value, colonMask, true);
}
void UsermodTM1637::addToConfig(JsonObject& root) {
  JsonObject top = root.createNestedObject(CFG_NODE());
  top[CFG_CLK()]    = pinClk;
  top[CFG_DIO()]    = pinDio;
  top[CFG_BRIGHT()] = displayBrightness;
  top[CFG_PULSEMAX()] = pulseMax;
  top[CFG_PULSPER()]  = pulsePeriod;
  top[CFG_SHOW()]     = showClock;
}
bool UsermodTM1637::readFromConfig(JsonObject& root) {
  JsonObject top = root[CFG_NODE()];
  if (top.isNull()) return false;
  pinClk = top[CFG_CLK()] | pinClk;
  pinDio = top[CFG_DIO()] | pinDio;
  displayBrightness = top[CFG_BRIGHT()] | displayBrightness;
  pulseMax          = top[CFG_PULSEMAX()] | pulseMax;
  pulsePeriod       = top[CFG_PULSPER()]  | pulsePeriod;
  showClock         = top[CFG_SHOW()]     | showClock;
  if (display) { delete display; display = nullptr; }
  initialized = false;
  return true;
}
void UsermodTM1637::addToJsonInfo(JsonObject& root) {
  JsonObject user = root[F("u")];
  if (user.isNull()) user = root.createNestedObject(F("u"));
  JsonArray arr = user.createNestedArray(F("TM1637"));
  arr.add(F("Uhr: HH:MM (pulsierender Doppelpunkt)"));
}


static UsermodTM1637 usermodtm1637;
REGISTER_USERMOD(usermodtm1637);