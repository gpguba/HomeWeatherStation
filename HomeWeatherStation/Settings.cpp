#include "Settings.h"

Settings::Settings(){

}

void Settings::load(){
  ssidLen = EEPROM.read(ssidLenAdr);
  passwordLen = EEPROM.read(passwordLenAdr);

  if(ssidLen < 1 || ssidLen > 32 || passwordLen < 8 && passwordLen != 0  || passwordLen > 32) reset();
  else{
    ssid = "";
    password = "";
    for(int i=0;i<ssidLen;i++) ssid += (char)EEPROM.read(ssidAdr+i);
    for(int i=0;i<passwordLen;i++) password += (char)EEPROM.read(passwordAdr+i);

    ssidHidden = (bool)EEPROM.read(ssidHiddenAdr);

  }
}

void Settings::reset(){
  if(debug) Serial.print("reset settings...");

  ssid = "esp8266";
  password = "00000000"; //must have at least 8 characters
  ssidHidden = false;

  ssidLen = ssid.length();
  passwordLen = password.length();

  if(debug) Serial.println("done");

  save();
}

void Settings::save(){
  ssidLen = ssid.length();
  passwordLen = password.length();

  EEPROM.write(ssidLenAdr, ssidLen);
  EEPROM.write(passwordLenAdr, passwordLen);
  for(int i=0;i<ssidLen;i++) EEPROM.write(ssidAdr+i,ssid[i]);
  for(int i=0;i<passwordLen;i++) EEPROM.write(passwordAdr+i,password[i]);

  EEPROM.write(ssidHiddenAdr, ssidHidden);
  // eepromWriteInt(attackTimeoutAdr, attackTimeout);

  EEPROM.commit();

  if(debug){
    info();
    Serial.println("settings saved");
  }
}

void Settings::info(){
  Serial.println("settings:");
  Serial.println("SSID: "+ssid);
  Serial.println("SSID length: "+(String)ssidLen);
  Serial.println("SSID hidden: "+(String)ssidHidden);
  Serial.println("password: "+password);
  Serial.println("password length: "+(String)passwordLen);
}

String Settings::get(){
  if(debug) Serial.println("getting settings json");
  String json = "{";

  json += "\"ssid\":\""+ssid+"\",";
  json += "\"ssidHidden\":"+(String)ssidHidden+",";
  json += "\"password\":\""+password+"\"";
  json += "] }";
  if(debug){
    Serial.println(json);
    Serial.println("done");
  }
  return json;
}
