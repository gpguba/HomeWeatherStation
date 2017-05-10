#ifndef Settings_h
#define Settings_h

#include <EEPROM.h>
#include <Arduino.h>

extern bool debug;
// extern const bool debug;
// extern void eepromWriteInt(int adr, int val);
// extern int eepromReadInt(int adr);

#define ssidLenAdr 1024
#define ssidAdr 1025
#define passwordLenAdr 1057
#define passwordAdr 1058
#define ssidHiddenAdr 1090

class Settings
{
  public:
    Settings();
    void load();
    void reset();
    void save();
    String get();
    void info();

    int ssidLen;
    String ssid = "";
    bool ssidHidden;
    int passwordLen;
    String password = "";

  private:
};

#endif
