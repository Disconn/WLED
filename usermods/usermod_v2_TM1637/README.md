# WLED-Usermod-TM1637 — Temperature-Style Split
- getId()/getName()
- PROGMEM/F()-Configkeys
- Deferred init & pin guards

## Setup
- Ordner in `lib/WLED-Usermod-TM1637/`
- `examples/MinimalRegister/usermods_list.cpp` nach `wled00/` kopieren/anpassen
- `platformio.ini`:
  ```ini
  build_unflags = -D WLED_DISABLE_USERMODS
  build_flags = -D USERMOD_TM1637
  lib_deps = ${esp32.lib_deps}
             https://github.com/avishorp/TM1637.git
  ```
