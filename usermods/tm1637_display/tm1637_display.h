#pragma once
// ============================================================================
//  TM1637 4‑Digit Display Usermod für WLED (v2 API)
//  Stabil & UI‑freundlich:
//   - Text ODER Uhrzeit (HH:MM) per Bool (show_time__bool)
//   - Pulsierender Doppelpunkt (show_colon/colon_pulse/period/duty)
//   - GPIOs & Helligkeit (0–7), optional an WLED‑Brightness koppeln
//   - Web‑UI Beschreibungen via "~"
//   - Saubere Trennung: Header (.h) + Implementierung (.cpp)
// ============================================================================

#include "wled.h"

// 7‑Segment LUT (a..g,dp); DP ungenutzt, Doppelpunkt eigenes Bit an Digit1
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
  uint8_t pinCLK                = 18;
  uint8_t pinDIO                = 19;
  uint8_t brightness            = 3;      // 0..7
  bool    followWledBrightness  = false;
  bool    showColon             = true;
  bool    colonPulse            = true;
  uint16_t colonPulsePeriodMs   = 1500;
  uint8_t  colonDutyMinPct      = 10;
  uint8_t  colonDutyMaxPct      = 90;

  // Zeit-Anzeige Optionen
  bool    showTime              = false;  // false=text, true=time
  bool    time24h               = true;
  bool    timeLeadingZero       = true;
  bool    blankUntilTimeValid   = false;  // Display bleibt leer bis NTP-Zeit gültig
  bool    colonBlinkSeconds     = false;  // im Zeitmodus: Sekundentakt statt Puls

  // Optionale SNTP-Initialisierung (nur wenn WLED keine Zeit liefert)
  bool    forceTimeInit         = false;  // bei Zeit=ungültig selbst SNTP starten
  String  tzString              = "";     // z.B. "CET-1CEST,M3.5.0/2,M10.5.0/3"
  String  ntpServer1            = "";     // z.B. "pool.ntp.org"
  String  ntpServer2            = "";     // optional zweiter Server

  // Freitext für Modus=Text (4 Zeichen; ':' wird ignoriert)
  String  content               = "-- --"; // 4 Zeichen, ':' ignoriert

   // 4 Zeichen, ':' ignoriert

  UsermodTM1637();

  void setup() override;
  void connected() override;
  void loop() override;

  // Info-/State-/Config‑Schnittstellen
  void addToJsonInfo(JsonObject& root) override;
  bool handleUsermod(JsonObject umData);
  void addToConfig(JsonObject &root) override;
  bool readFromConfig(JsonObject &root) override;
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
  void readClock(int &h, int &m, int &s, bool &valid);
  void initTimeIfNeeded();
};
