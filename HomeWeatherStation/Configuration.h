
#include <Arduino.h>
#include <WiFiClient.h>
//temperature and humidity sensor (HTU21D) params
#define HTDU21D_ADDRESS 0x40  //Unshifted 7-bit I2C address for the sensor
#define TRIGGER_TEMP_MEASURE_HOLD  0xE3
#define TRIGGER_HUMD_MEASURE_HOLD  0xE5
#define TRIGGER_TEMP_MEASURE_NOHOLD  0xF3
#define TRIGGER_HUMD_MEASURE_NOHOLD  0xF5
#define WRITE_USER_REG  0xE6
#define READ_USER_REG  0xE7
#define SOFT_RESET  0xFE

//pins
#define LED_BUILTIN 13
#define RELAY_PIN D6

//thingspeak

#define CHANEL_ID
#define READ_API_KEY
#define WRITE_API_KEY
int tspkStatus;
//timers
unsigned long tspkTimer=0;
unsigned long lightSensorTimer=0;
unsigned long tempSensorTimer=0;
unsigned int lum=0;
unsigned int tem=0;



//MQTT
#define MQTT_SERVER       "io.adafruit.com"
#define MQTT_SERVERPORT   8883
#define MQTT_USERNAME     "gpguba"
#define MQTT_KEY
#define USERNAME          "gpguba/"
#define PREAMBLE          "feeds/"
#define T_LUMINOSITY      "luminosity"
#define T_CLIENTSTATUS    "clientStatus"
#define T_TEMPERATURE     "temperature"
#define T_HUMIDITY        "humidity"
#define T_COMMAND         "onoff"


//light sensor (TSL2561) params
boolean gain;     // Gain setting, 0 = X1, 1 = X16;
unsigned int ms;  // Integration ("shutter") time in milliseconds

//wifi
const char* ssid =        
const char* password =    
const char* host =        


//used for static ip but will not work together with mDNS
// IPAddress ip(192,168,1,100);
// IPAddress gateway(192,168,1,1);
// IPAddress subnet(255,255,255,0);

int timer=0;
String serialData ="";
