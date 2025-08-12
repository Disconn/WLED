#pragma once
// ============================================================================
//  TM1637 4‑Digit Display Usermod for WLED
//  
// ============================================================================

#include "wled.h"

extern const uint8_t SEGMENTS_LUT[16];

// --- Low‑Level TM1637 Bus ---
class TM1637Bus {
public:
  TM1637Bus(uint8_t clk, uint8_t dio);
  void begin();
  void setBrightness(uint8_t level, bool on=true);
  void setSegments(const uint8_t* segs, uint8_t length, uint8_t pos=0);
  void clear();
private:
  uint8_t _clk, _dio;
  void start();
  void stop();
  bool writeByte(uint8_t b);
  void clkHigh(); void clkLow();
  void dioHigh(); void dioLow();
  void dioIn();   void dioOut();
};

// --- Usermod ---
class UsermodTM1637 : public Usermod {
public:
  // Konfigurierbar
  bool    enabled               = true;
  int8_t pinCLK                = 18;
  int8_t pinDIO                = 19;
  uint8_t brightness            = 3;      // 0..7
  bool    followWledBrightness  = false;
  bool    showColon             = true;

  // Zeit-Anzeige Optionen
  bool    showTime              = false;  // false=text, true=time
  bool    time24h               = true;
  bool    timeLeadingZero       = true;
  bool    colonBlinkSeconds     = true;  // im Zeitmodus: Sekundentakt statt Puls
  bool    showIP                = true;
  

  // Freitext für Modus=Text (4 Zeichen; ':' wird ignoriert)
  String  content               = "0123"; 

  UsermodTM1637();

  void setup() override;
  void connected() override;
  void loop() override;

  // Info-/State-/Config‑Schnittstellen
  void addToJsonInfo(JsonObject& root) override;
  bool handleUsermod(JsonObject umData);
  void addToConfig(JsonObject &root) override;
  bool readFromConfig(JsonObject &root) override;
  bool getLocalIP(int ipOut[4]) const;
  uint16_t getId() override { return 0x12A7; }

  void setContent(const String &s);

private:
  TM1637Bus _bus = TM1637Bus(pinCLK, pinDIO);
  unsigned long _lastUpdate = 0;
  uint8_t _appliedBrightness = 0xff;
  bool _colonRenderState = false;

  void applyBrightness();
  uint8_t mapChar(char c);
  void updateColon(unsigned long nowMs);
  void renderText();
  void renderTime();
  void readClock(int &h, int &m, int &s);
  void initTimeIfNeeded();
};
