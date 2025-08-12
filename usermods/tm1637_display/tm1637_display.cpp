#include "tm1637_display.h"
#include <math.h>   // cosf,fmodf
#include <time.h>   
#ifdef ARDUINO_ARCH_ESP32
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
  me[F("mode")]      = showTime ? F("time") : F("text");
  me[F("time24h")]   = time24h;
  me[F("lead0")]     = timeLeadingZero;
  me[F("content")]   = content;
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
  if (o.containsKey(F("brightness"))) { brightness = constrain((int)o[F("brightness")],0,7); applyBrightness(); changed = true; }

  return changed;
}

// Web‑UI
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

  // Pins
  top[F("gpio_clk")] = pinCLK;
  top[F("gpio_dio")] = pinDIO;

  // Helligkeit
  top[F("brightness_level")] = brightness;
  top[F("follow_wled_brightness")] = followWledBrightness;

  // Doppelpunkt
  top[F("show_colon")] = showColon;
  top[F("colon_blink_seconds")] = colonBlinkSeconds;

  top[F("show_ip")] = showIP;
}

bool UsermodTM1637::getLocalIP(int ipOut[4]) const {
  IPAddress ip(0,0,0,0);

#if defined(ARDUINO_ARCH_ESP32)

  if (WiFi.isConnected()) {
    ip = WiFi.localIP();
  }

  if ((ip[0] | ip[1] | ip[2] | ip[3]) == 0 && ETH.linkUp()) {
    IPAddress ethIp = ETH.localIP();
    if ((ethIp[0] | ethIp[1] | ethIp[2] | ethIp[3]) != 0) {
      ip = ethIp;
    }
  }
#else

#endif

  ipOut[0] = ip[0];
  ipOut[1] = ip[1];
  ipOut[2] = ip[2];
  ipOut[3] = ip[3];

  return (ip[0] | ip[1] | ip[2] | ip[3]) != 0;
}

bool UsermodTM1637::readFromConfig(JsonObject &root){
  JsonObject top = root[F("tm1637_display")];
  if (top.isNull()) return false;

  enabled               = top[F("enabled")] | enabled;
  showTime              = top[F("show_time")] | showTime;
  if (top.containsKey(F("content"))) content = top[F("content")] | content;

  time24h               = top[F("time_24h")] | time24h;
  timeLeadingZero       = top[F("time_leading_zero")] | timeLeadingZero;

  pinCLK                = top[F("gpio_clk")] | pinCLK;
  colonBlinkSeconds     = top[F("colon_blink_seconds")] | colonBlinkSeconds;
  pinDIO                = top[F("gpio_dio")] | pinDIO;

  brightness            = constrain((int)(top[F("brightness_level")] | brightness), 0, 7);
  followWledBrightness  = top[F("follow_wled_brightness")] | followWledBrightness;

  showColon             = top[F("show_colon")] | showColon;
  showIP                = top[F("show_ip")] | showIP;

  // Re‑init
  _bus = TM1637Bus(pinCLK, pinDIO);
  if (enabled){
    _bus.begin();
    applyBrightness();
    if (showTime) renderTime(); else renderText();
  }
  return true;
}


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

void UsermodTM1637::renderText(){
  int s=0; 
  s = second(localTime);

  if(showIP) {
    int s1 = s % 10;
    int ip[4] = {0,0,0,0};
    bool ok = getLocalIP(ip);
    if(s1 == 0 || s1 == 1) {
      content = ip[0];
    }
    if(s1 == 2 || s1 == 3) {
      content = ip[1];
    }
    if(s1 == 4 || s1 == 5) {
      content = ip[2];
    }
    if(s1 == 6 || s1 == 7) {
      content = ip[3];
    }
    if(s1 == 8 || s1 == 9) {
      content = "    ";
    }
  }

  uint8_t seg[4]={0,0,0,0};
  char buf[5]={0,0,0,0,0};
  uint8_t idx=0;
  for(size_t i=0;i<content.length() && idx<4;i++){ char c=content.charAt(i); if(c==':') continue; buf[idx++]=c; }
  while(idx<4) buf[idx++]=' ';
  for(uint8_t i=0;i<4;i++) seg[i]=mapChar(buf[i]);
  if (_colonRenderState) seg[1] |= 0x80;
  _bus.setSegments(seg,4,0);


  if (colonBlinkSeconds && showColon){
    _colonRenderState = (s % 2 == 0);
  } else {
    if(showColon){
      _colonRenderState = 1;
    } else {
      _colonRenderState = 0;
    }
  }
}

void UsermodTM1637::renderTime(){
  int h=0,m=0,s=0;

  h = hour(localTime); m = minute(localTime); s = second(localTime);

  int hh=h;
  if (!time24h){ hh = hh%12; if(hh==0) hh=12; }

  char out[5]={0,0,0,0,0};
  if (timeLeadingZero || hh>=10) snprintf(out,sizeof(out),"%02d%02d",hh,m);
  else snprintf(out,sizeof(out)," %1d%02d",hh,m);

  content = String(out);

  if (colonBlinkSeconds && showColon){
    _colonRenderState = (s % 2 == 0); 
  }

  renderText();
}

static UsermodTM1637 usermodtm1637;
REGISTER_USERMOD(usermodtm1637);