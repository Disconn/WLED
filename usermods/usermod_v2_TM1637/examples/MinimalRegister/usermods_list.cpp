
// examples/MinimalRegister/usermods_list.cpp
#include "wled.h"
#ifdef USERMOD_TM1637
  #include "usermods/usermod_tm1637.h"
#endif
void registerUsermods() {
  #ifdef USERMOD_TM1637
    usermods.add(new UsermodTM1637());
  #endif
}
