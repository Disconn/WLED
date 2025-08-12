#include "tm1637_display.h"
#include <math.h>   // cosf,fmodf
#include <time.h>   // time(), localtime_r, getLocalTime (ESP32)
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#endif

// LUT
const uint8_t SEGMENTS_LUT[16] = {
  0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,
  0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71
};

// --- Bus ---
TM1637Bus::TM1637Bus(uint8_t clk, uint8_t dio) : _clk(clk), _dio(dio) {}
void TM1637Bus::clkHigh(){ digitalWrite(_clk,HIGH);} void TM1637Bus::clkLow(){ digitalWrite(_clk,LOW);}
void TM1637Bus::dioHigh(){ digitalWrite(_dio,HIGH);} void TM1637Bus::dioLow(){ digitalWrite(_dio,LOW);}
void TM1637Bus::dioIn(){ pinMode(_dio, INPUT_PULLUP);} void TM1637Bus::dioOut(){ pinMode(_dio, OUTPUT);}

void TM1637Bus::begin(){
  pinMode(_clk, OUTPUT); pinMode(_dio, OUTPUT);
  digitalWrite(_clk, HIGH); digitalWrite(_dio, HIGH);
}
void TM1637Bus::start(){ dioHigh(); clkHigh(); delayMicroseconds(2); dioLow(); delayMicroseconds(2); clkLow(); delayMicroseconds(2); }
void TM1637Bus::stop(){ clkLow(); delayMicroseconds(2); dioLow(); delayMicroseconds(2); clkHigh(); delayMicroseconds(2); dioHigh(); delayMicroseconds(2); }
bool TM1637Bus::writeByte(uint8_t b){
  for(uint8_t i=0;i<8;i++){ clkLow(); delayMicroseconds(2); (b&1)?dioHigh():dioLow(); delayMicroseconds(2); clkHigh(); delayMicroseconds(2); b>>=1; }
  clkLow(); delayMicroseconds(2); dioIn(); delayMicroseconds(2); clkHigh(); delayMicroseconds(2);
  bool ack = !digitalRead(_dio); clkLow(); delayMicroseconds(2); dioOut(); return ack;
}
void TM1637Bus::setBrightness(uint8_t level, bool on){
  if(level>7) level=7; start(); writeByte(0x88 | (on?0x08:0x00) | level); stop();
}
void TM1637Bus::setSegments(const uint8_t* segs, uint8_t length, uint8_t pos){
  start(); writeByte(0x40); stop();
  start(); writeByte(0xC0 | pos); for(uint8_t i=0;i<length;i++) writeByte(segs[i]); stop();
}
void TM1637Bus::clear(){ uint8_t b[4]={0,0,0,0}; setSegments(b,4,0); }

// --- Usermod ---
UsermodTM1637::UsermodTM1637() : _bus(pinCLK, pinDIO) {}

void UsermodTM1637::setup(){
  if(!enabled) return;
  _bus = TM1637Bus(pinCLK,pinDIO);
  _bus.begin();
  applyBrightness();
  _lastUpdate = millis();
  if (showTime) renderTime(); else renderText();
}

void UsermodTM1637::connected(){}

void UsermodTM1637::loop(){
  if(!enabled) return;

  if (followWledBrightness){
    uint8_t mapped = map((uint16_t)bri, 0, 255, 0, 7);
    if (mapped != _appliedBrightness){ brightness = mapped; applyBrightness(); }
  } else {
    uint8_t mapped = map((uint16_t)brightness, 0, 255, 0, 7);
    if (mapped != _appliedBrightness){ applyBrightness(); }
  }

  unsigned long nowMs = millis();
  if (nowMs - _lastUpdate >= 25){
    _lastUpdate = nowMs;
    updateColon(nowMs);
    if (showTime) renderTime(); else renderText();
  }
}

void UsermodTM1637::setContent(const String &s){ content = s; if(!showTime) renderText(); }

void UsermodTM1637::addToJsonInfo(JsonObject& root){
  JsonObject u = root["u"].isNull() ? root.createNestedObject("u") : root["u"].as<JsonObject>();
  JsonObject me = u.createNestedObject(F("TM1637 Display"));
  me["enabled"]      = enabled;
  me[F("gpio_clk")]  = pinCLK;
  me[F("gpio_dio")]  = pinDIO;
  me[F("brightness")] = brightness;
  me[F("colon")]     = showColon;
  me[F("colonPulse")] = colonPulse;
  me[F("mode")]      = showTime ? F("time") : F("text");
  me[F("time24h")]   = time24h;
  me[F("lead0")]     = timeLeadingZero;
  me[F("content")]   = content;
  // Zeitstatus
  time_t __ts = time(nullptr);
  bool __time_valid = (__ts != (time_t)-1 && __ts >= 60);
  me["time_valid"] = __time_valid;
}

bool UsermodTM1637::handleUsermod(JsonObject umData){
  bool changed=false;
  if (!umData.containsKey(F("tm1637"))) return false;
  JsonObject o = umData[F("tm1637")];

  if (o.containsKey(F("mode"))) {
    String m = o[F("mode")].as<String>(); m.toLowerCase();
    showTime = (m==F("time"));
    changed = true;
  }
  if (o.containsKey(F("show_time"))) { showTime = o[F("show_time")]; changed = true; }

  if (o.containsKey(F("content"))) { setContent(o[F("content")].as<String>()); changed = true; }
  if (o.containsKey(F("time24h"))) { time24h = o[F("time24h")]; changed = true; }
  if (o.containsKey(F("timeLeadingZero"))) { timeLeadingZero = o[F("timeLeadingZero")]; changed = true; }

  if (o.containsKey(F("colon"))) { showColon = o[F("colon")]; changed = true; }
  if (o.containsKey(F("colonPulse"))) { colonPulse = o[F("colonPulse")]; changed = true; }
  if (o.containsKey(F("brightness"))) { brightness = constrain((int)o[F("brightness")],0,7); applyBrightness(); changed = true; }

  return changed;
}

// Web‑UI (stabil: nur "~" Beschreibungen)
void UsermodTM1637::addToConfig(JsonObject &root){
  JsonObject top = root.createNestedObject(F("tm1637_display"));

  top[F("enabled")] = enabled;

  // Modus
  top[F("show_time")] = showTime;

  // Text (nur wenn show_time=0 genutzt)
  top[F("content")] = content;

  // Zeitformat
  top[F("time_24h")] = time24h;
  top[F("time_leading_zero")]  = timeLeadingZero;

  // Anzeigeverhalten
  top[F("blank_until_time_valid")] = blankUntilTimeValid;
  top[F("colon_blink_seconds")] = colonBlinkSeconds;

  // Pins
  top[F("gpio_clk__gpio")] = pinCLK;
  top[F("gpio_dio__gpio")] = pinDIO;

  // Helligkeit
  top[F("brightness__level")] = brightness;
  top[F("follow_wled_brightness")] = followWledBrightness;

  // Doppelpunkt
  top[F("show_colon")] = showColon;
  top[F("colon_pulse")] = colonPulse;
  top[F("colon_pulse_period_ms__ms")] = colonPulsePeriodMs;
  top[F("colon_duty_min_pct__pct")] = colonDutyMinPct;
  top[F("colon_duty_max_pct__pct")] = colonDutyMaxPct;
}

bool UsermodTM1637::readFromConfig(JsonObject &root){
  JsonObject top = root[F("tm1637_display")];
  if (top.isNull()) return false;

  enabled               = top[F("enabled")] | enabled;
  showTime              = top[F("show_time")] | showTime;
  if (top.containsKey(F("content"))) content = top[F("content")] | content;

  time24h               = top[F("time_24h")] | time24h;
  timeLeadingZero       = top[F("time_leading_zero")] | timeLeadingZero;

  pinCLK                = top[F("gpio_clk__gpio")] | pinCLK;
  blankUntilTimeValid   = top[F("blank_until_time_valid")] | blankUntilTimeValid;
  colonBlinkSeconds     = top[F("colon_blink_seconds")] | colonBlinkSeconds;
  pinDIO                = top[F("gpio_dio__gpio")] | pinDIO;

  brightness            = constrain((int)(top[F("brightness__level")] | brightness), 0, 7);
  followWledBrightness  = top[F("follow_wled_brightness")] | followWledBrightness;

  showColon             = top[F("show_colon")] | showColon;
  colonPulse            = top[F("colon_pulse")] | colonPulse;
  colonPulsePeriodMs    = top[F("colon_pulse_period_ms")] | colonPulsePeriodMs;
  colonDutyMinPct       = constrain((int)(top[F("colon_duty_min")] | colonDutyMinPct), 0, 100);
  colonDutyMaxPct       = constrain((int)(top[F("colon_duty_max")] | colonDutyMaxPct), 0, 100);

  // Re‑init
  _bus = TM1637Bus(pinCLK, pinDIO);
  if (enabled){
    _bus.begin();
    applyBrightness();
    if (showTime) renderTime(); else renderText();
  }
  return true;
}

// --- intern ---
void UsermodTM1637::applyBrightness(){
  _appliedBrightness = constrain(brightness, 0, 7);
  _bus.setBrightness(_appliedBrightness, true);
}

uint8_t UsermodTM1637::mapChar(char c){
  if (c>='0' && c<='9') return SEGMENTS_LUT[c-'0'];
  if (c>='A' && c<='F') return SEGMENTS_LUT[10+(c-'A')];
  if (c>='a' && c<='f') return SEGMENTS_LUT[10+(c-'a')];
  if (c=='-') return 0x40;
  if (c==' ') return 0x00;
  return 0x00;
}

void UsermodTM1637::updateColon(unsigned long nowMs){
  if (!showColon){ _colonRenderState=false; return; }
  if (!colonPulse || colonPulsePeriodMs < 50){ _colonRenderState=true; return; }

  float phase = fmodf((float)(nowMs % colonPulsePeriodMs), (float)colonPulsePeriodMs) / (float)colonPulsePeriodMs;
  float y = 0.5f - 0.5f * cosf(phase * 2.0f * (float)PI); // weich

  uint8_t dmin = colonDutyMinPct, dmax = colonDutyMaxPct;
  if (dmin > dmax){ uint8_t t=dmin; dmin=dmax; dmax=t; }
  float duty = dmin + (dmax - dmin) * y;

  const uint16_t sub = 200;
  _colonRenderState = ((nowMs % sub) < (uint16_t)(sub * (duty/100.0f)));
}

void UsermodTM1637::renderText(){
  uint8_t seg[4]={0,0,0,0};
  char buf[5]={0,0,0,0,0};
  uint8_t idx=0;
  for(size_t i=0;i<content.length() && idx<4;i++){ char c=content.charAt(i); if(c==':') continue; buf[idx++]=c; }
  while(idx<4) buf[idx++]=' ';
  for(uint8_t i=0;i<4;i++) seg[i]=mapChar(buf[i]);
  if (_colonRenderState) seg[1] |= 0x80;
  _bus.setSegments(seg,4,0);
}

void UsermodTM1637::renderTime(){
  int h=0,m=0,s=0; bool valid=false; readClock(h,m,s,valid);

  if (blankUntilTimeValid && !valid){
    // leer anzeigen
    uint8_t seg[4]={0,0,0,0};
    _bus.setSegments(seg,4,0);
    return;
  }

  int hh=h;
  if (!time24h){ hh = hh%12; if(hh==0) hh=12; }

  char out[5]={0,0,0,0,0};
  if (timeLeadingZero || hh>=10) snprintf(out,sizeof(out),"%02d%02d",hh,m);
  else snprintf(out,sizeof(out)," %1d%02d",hh,m);

  // Content aktualisieren
  content = String(out);

  // Doppelpunkt: entweder Sekundentakt oder Puls
  if (colonBlinkSeconds && valid && showColon){
    _colonRenderState = (s % 2 == 0); // an in geraden Sekunden
  }

  renderText();
}

void UsermodTM1637::readClock(int &h, int &m, int &s, bool &valid){
  valid = false;

  h = hour(localTime); m = minute(localTime); s = second(localTime); valid = true; DEBUG_PRINTLN(F("getLocalTime used"));

}

static UsermodTM1637 temperature2;
REGISTER_USERMOD(temperature2);