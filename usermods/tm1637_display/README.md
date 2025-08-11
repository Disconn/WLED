# TM1637 4‑Digit Display Usermod (stabile Web‑UI Keys)

**Features:** Text- oder Uhrzeitanzeige (HH:MM), pulsierender Doppelpunkt, konfigurierbare GPIOs & Helligkeit, optionale Kopplung an WLED‑Brightness.

**Stabile UI‑Keys:** Nur `~` Beschreibungen, keine exotischen `__sel`/`-`/`+` Zusätze nötig. 

## Wichtige JSON/Config Keys
- `tm1637_display.enabled__bool` – 0/1
- `tm1637_display.show_time__bool` – 1 = Uhrzeit, 0 = Text
- `tm1637_display.content__text` – 4 Zeichen (Textmodus)
- `tm1637_display.time_24h__bool`, `time_leading_zero__bool`
- `tm1637_display.gpio_clk__gpio`, `gpio_dio__gpio`
- `tm1637_display.brightness__level` (0–7), `follow_wled_brightness__bool`
- `tm1637_display.show_colon__bool`, `colon_pulse__bool`, `colon_pulse_period_ms__ms`
- `tm1637_display.colon_duty_min_pct__pct`, `colon_duty_max_pct__pct`

## JSON Beispiele
```json
{"usermod":{"tm1637":{"show_time":true}}}
{"usermod":{"tm1637":{"show_time":false,"content":"12:34"}}}
{"usermod":{"tm1637":{"time24h":true,"timeLeadingZero":false}}}
{"usermod":{"tm1637":{"brightness":6,"colon":true,"colonPulse":true}}}
```
