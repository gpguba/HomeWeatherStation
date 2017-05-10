#include <Arduino.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include "Configuration.h"

extern bool debug;
typedef struct
{
  unsigned int visibleSpectrum;
  unsigned int infrared;
  double lux;
  float temp;
  float humidity;
  bool tempUpdated;
} SensorData;

// WiFiClient client;
// WiFiFlientSecure for SSL/TLS support
WiFiClientSecure client;

// io.adafruit.com SHA1 fingerprint
const char* fingerprint = "26 96 1C 2A 51 07 FD 15 80 96 93 AE F7 32 CE B9 0D 01 55 C4";


// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
// const char LUMINOSITY_FEED[] PROGMEM = USERNAME  PREAMBLE  T_LUMINOSITY;
Adafruit_MQTT_Publish tLuminosity = Adafruit_MQTT_Publish(&mqtt, USERNAME PREAMBLE T_LUMINOSITY);

// Setup a feed called 'clientStatus' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
// const char CLIENTSTATUS_FEED[] PROGMEM = USERNAME  PREAMBLE  T_CLIENTSTATUS;
Adafruit_MQTT_Publish tTemperature = Adafruit_MQTT_Publish(&mqtt, USERNAME  PREAMBLE  T_TEMPERATURE);
Adafruit_MQTT_Publish tHumidity = Adafruit_MQTT_Publish(&mqtt, USERNAME  PREAMBLE  T_HUMIDITY);

// Setup a feed called 'command' for subscribing to changes.
// const char COMMAND_FEED[] PROGMEM = USERNAME  PREAMBLE  T_COMMAND;
Adafruit_MQTT_Subscribe tCommand = Adafruit_MQTT_Subscribe(&mqtt, USERNAME  PREAMBLE  T_COMMAND);

void connectMQTT();
void verifyFingerprint();

void connectMQTT() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}


bool publishLuminosity(float value) {
  // Now we can publish stuff!
  bool success = false;
  if(debug){
    Serial.print(F("\nSending Luminosity "));
    Serial.print(value);
    Serial.print("...");
  }
  if (! tLuminosity.publish(value)) {
    if(debug)Serial.println(F("Failed"));
  } else {
    if(debug)Serial.println(F("OK!"));
  }
  return success;
}

bool publishTemperature(float value) {
  // Now we can publish stuff!
  bool success = false;
  if(debug){
    Serial.print(F("\nSending Temperature "));
    Serial.print(value);
    Serial.print("...");
  }
  if (! tTemperature.publish(value)) {
    if(debug)Serial.println(F("Failed"));
  } else {
    if(debug)Serial.println(F("OK!"));
  }
  return success;
}
bool publishHumidity(float value) {
  // Now we can publish stuff!
  bool success = false;
  if(debug){
    Serial.print(F("\nSending Humidity "));
    Serial.print(value);
    Serial.print("...");
  }
  if (! tHumidity.publish(value)) {
    if(debug)Serial.println(F("Failed"));
  } else {
    if(debug)Serial.println(F("OK!"));
  }
  return success;
}

// bool publishClientStatus(bool clientStat) {
//   bool success = false;
//   Serial.print(F("\nSending clientStatus "));
//   Serial.print(clientStat);
//   Serial.print("...");
//
//   if (! tClientStatus.publish(clientStat, 1)) Serial.println(F("Failed"));
//   else {
//     Serial.println(F(" OK!"));
//     success = true;
//   }
//   return success;
// }

void receiveCommand() {
  // this is our 'wait for incoming subscription packets' busy subloop
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(1000))) {
    Serial.print(F("Got something "));
    if (subscription == &tCommand) {
      Serial.print(F("Got: "));
      Serial.println((char*)tCommand.lastread);

      // Switch on the LED if an 1 was received as first character
      if (tCommand.lastread[1] == 'N') {

        digitalWrite(RELAY_PIN, HIGH);
        //digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
        // but actually the LED is on; this is because
        // it is acive low on the ESP-01)
      }
      if (tCommand.lastread[1] == 'F') {
        digitalWrite(RELAY_PIN, LOW);
        //digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
      }

    }
  //   // ping the server to keep the mqtt connection alive
  // if(! mqtt.ping()) {
  //   mqtt.disconnect();
  // }


  }
}
void verifyFingerprint() {

  const char* host = MQTT_SERVER;

  Serial.print("Connecting to ");
  Serial.println(host);

  if (! client.connect(host, MQTT_SERVERPORT)) {
    Serial.println("Connection failed. Halting execution.");
    while(1);
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("Connection secure.");
  } else {
    Serial.println("Connection insecure! Halting execution.");
    while(1);
  }

}
