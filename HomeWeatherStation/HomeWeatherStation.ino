#include <Arduino.h>
#include <ArduinoOTA.h>
#include <SparkFunTSL2561.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h> //for test
#include <Ticker.h>
#include "MQTTSetup.h"
//#include <avr/pgmspace.h> //for test2
#include "Settings.h"
#include "ThingSpeak.h"

/* ========== DEBUG ========== */
bool debug = true;
// const bool debug = true;
/* ========== DEBUG ========== */

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiClient wifiClient;
WiFiClient wifiClient2;
Ticker ticker;
String page = "";
bool webDebug = true;
// Create an SFE_TSL2561 object, here called "light":
SFE_TSL2561 light;
SensorData data;

volatile int ISRTimer = 0;
void ISRwatchdog(){
  ISRTimer++;
  if(ISRTimer > 60){
    Serial.println(dataAppender("Soft Resetting..."));
    ESP.reset();
  }
}
void serialMonitor() {
  webDebug = true;
  debug = true;
  String webpage = "<!DOCTYPE html><html><head>";
  // webpage += "<meta http-equiv=\"refresh\" content=\"5\">"; //refresh rate per sec
  // webpage += "<meta http-equiv=\"Connection\" content=\"close\">"; //close connection after page loads
  // webpage += "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">";
  webpage += "<meta content=\"text/html;charset=utf-8\">";
  webpage += "<title>Serial Monitor</title>";

  webpage += "</head>";
  webpage += "<script>function retrieveData(url)";
  webpage += "{    if(url=='/serial')setTimeout(function() {retrieveData(url);}, 5000);console.log(url); var xmlHttp = new XMLHttpRequest();";
  webpage += "\n    xmlHttp.onreadystatechange = function() {";//ALWAYS separate all functions in new line !!
  webpage += "         if (xmlHttp.readyState == 4 && xmlHttp.status == 200)";
  webpage += "            callback(xmlHttp.responseText); ";
  webpage += "   }"; //separate ending of function declaration with new line!!!
  webpage += "\n    xmlHttp.open(\"GET\", url, true);";
  webpage += " xmlHttp.send(null);";
  webpage += "}";
  webpage += "\nfunction callback(data){console.log(data); if(data.startsWith(\"**\")){document.getElementById('monitor').innerHTML = data;}else {document.getElementById('monitor').innerHTML += data}}";
  webpage += "</script>";
  webpage += "<body>";
  webpage += "<br><br><div><button  onclick=retrieveData(\"/serial\")> retrieve data </button><br>";
  webpage += "<br><br><div><button  onclick=retrieveData(\"/stopmonitor\")> stop webdebug </button>";
  webpage += "<div><button  onclick=retrieveData(\"/stopdebug\")> stop debug </button>";
  webpage += "<div id=\"monitor\"></div>";
  webpage += "<script>retrieveData(\"/serial\");</script>";

  webpage += "</body>";
  // webpage += "<div id=\"footer\">Copyright &copy; G P Guba</div>";
  server.send(200, "text/html", webpage);
}
String dataAppender(String dat){
  if(webDebug)serialData += dat +"<br>";
  // Serial.print(serialData);
  return dat;
}
void generateSerialData(){
  server.send(200, "text/plain", serialData);
  serialData = "";
}
void setup()
{
  if(debug) Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT); // LED_BUILTIN = 13
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  startWifi();
  if (MDNS.begin(host)) {
    Serial.println(dataAppender("MDNS responder started"));
  }
  server.on("/device", handleRoot);
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });
  server.on("/reset", restartESP);
  server.on("/serialmonitor", serialMonitor);
  server.on("/serial", generateSerialData);
  server.on("/stopmonitor", [](){
    webDebug = false;
    serialData = "";
    server.send(200, "text/plain", "STOPPED");
  });
  server.on("/stopdebug", [](){
    webDebug = false;
    debug = false;
    server.send(200, "text/plain", "DEBUG DISABLED");
  });
  server.onNotFound(handleNotFound);

  httpUpdater.setup(&server,"esp", "00000000"); // server,username,password
  server.begin();
  startOTAService();
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  Serial.println(dataAppender("HTTPUpdateServer ready!"));
  Serial.print("Open http://");
//  Serial.print(WiFi.localIP());
  Serial.print(host);
  Serial.print(".local/update in your browser\n");
  // queryMDNSServices();
  initLightSensor();

  verifyFingerprint();
  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&tCommand);

  ThingSpeak.begin(wifiClient);
  ticker.attach(1, ISRwatchdog);
  webDebug = false;
}

void initLightSensor(){
  // Initialize the SFE_TSL2561 library
  light.begin();

  // Get factory ID from sensor:
  // (Just for fun, you don't need to do this to operate the sensor)
  if(debug){
    unsigned char ID;

    if (light.getID(ID))
    {
      Serial.print("Light sensor factory ID: 0X");
      Serial.print(ID,HEX);
      Serial.println(", should be 0X5X");
    }
    else
    {
      byte error = light.getError();
      printError(error);
    }
  }
  // gain and time has default values 0 and 2 respctively. below is just to show how to customize
  gain = 0; //gain = 0 set to low gain (1x), gain = 1 set to high gain (16x)
  // If time = 0, integration will be 13.7ms; time = 1, 101ms; time = 2, 402ms;
  // time = 3 to use manual start / stop to perform your own integration and set different value for ms
  unsigned char time = 2;
  light.setTiming(gain,time,ms);
  light.setPowerUp();
}

// void timedFunction(int executeEveryXSec, int timeDelay, void func()){
//   int delayCoef = 0;
//   delayCoef = (int)executeEveryXSec*1000/timeDelay;
//   if(timer%delayCoef == 0){
//     func();
//     timer = 0; //reset
//   }
//   timer++;
//   delay(timeDelay);
// }


void loop()
{
  yield();
  server.handleClient();
  ArduinoOTA.handle();
  connectMQTT();
  receiveCommand();
  lum = millis();
  retrieveLuminosity(); //this process takes ~400ms
  if(debug)Serial.println("process time: " +(String)(millis() - lum));
  if ((millis() - lightSensorTimer > 200 || lightSensorTimer == 0)) {  // publish if value changed or after 200ms to keep connection alive
    if (publishLuminosity(data.lux))
    lightSensorTimer = millis();
  }

  if(debug){
    tem = millis();
    getTempAndHum(); //this process takes ~200ms
    Serial.println("process time: " +(String)(millis()-tem));
  }
  if(millis() - tempSensorTimer > 15000 || tempSensorTimer == 0){ //publish  temperature every 15 sec
    getTempAndHum(); //this process takes ~200ms
    if(debug)Serial.println(dataAppender("Publishing to Adafruit..."));
    publishTemperature(data.temp);
    publishHumidity(data.humidity);
  }

  // currentMillis = millis();
  // Serial.println(millis() - tspkTimer);
  if(millis() - tspkTimer  > 15*1000 || tspkTimer == 0){
    ThingSpeak.setField(1, data.temp);
    ThingSpeak.setField(2, data.humidity);
    ThingSpeak.setField(3, (float)data.lux);
    if(debug) Serial.print(dataAppender("Sending data to Thingspeak..."));
    tspkStatus = ThingSpeak.writeFields(CHANEL_ID,WRITE_API_KEY);
    if(debug) {
      if(tspkStatus == 200){
        Serial.println(dataAppender("OK"));
      } else {
        Serial.println("Failed error " + tspkStatus);
      }
    }
    tspkTimer = millis();

  }

  if(data.lux < 5) {
    digitalWrite(RELAY_PIN, LOW);
  }else{
    digitalWrite(RELAY_PIN, HIGH);
  }
  ISRTimer = 0;
  delay(500);
}

//SENSOR CONFIGURATION

void retrieveLuminosity(){
  delay(ms);

  // unsigned int data0, data1;

  if (light.getData(data.visibleSpectrum,data.infrared))
  {
    if(debug){
      // Serial.print(dataAppender((String)data.visibleSpectrum));
      // Serial.print(dataAppender(" infrared: "+(String)data.infrared));
      // Serial.print(dataAppender((String)data.infrared));
    }
    // double lux;    // Resulting lux value
    boolean good;  // True if neither sensor is saturated
    good = light.getLux(gain,ms,data.visibleSpectrum,data.infrared,data.lux);
    if(debug)Serial.println(dataAppender("visible: "+ (String)data.visibleSpectrum
      +" infrared: "+(String)data.infrared+" lux: "+String(data.lux)));

    // Print out the results:
    // if(debug){
    //   Serial.print(dataAppender(" lux: "));
    //   Serial.print(dataAppender(String(data.lux)));
    //   if (good) Serial.println(dataAppender(" (good)")); else Serial.println(dataAppender(" (BAD)"));
    // }
  }
  else
  {
    // getData() returned false because of an I2C error, inform the user.
    if(debug){
      byte error = light.getError();
      printError(error);
    }
  }
}

void printError(byte error)
{
  Serial.print("I2C error: ");
  Serial.print(error,DEC);
  Serial.print(", ");

  switch(error)
  {
    case 0:
      Serial.println("success");
      break;
    case 1:
      Serial.println("data too long for transmit buffer");
      break;
    case 2:
      Serial.println("received NACK on address (disconnected?)");
      break;
    case 3:
      Serial.println("received NACK on data");
      break;
    case 4:
      Serial.println("other error");
      break;
    default:
      Serial.println("unknown error");
  }
}

void getTempAndHum(){
  unsigned int rawHumidity = htdu21d_readHumidity();
  unsigned int rawTemperature = htdu21d_readTemp();

  // String shttemp = String (calc_temp(rawTemperature));
  // String shthum = String (calc_humidity(rawHumidity));
  data.temp = calc_temp(rawTemperature);
  data.humidity = calc_humidity(rawHumidity);
  data.tempUpdated = true;
  if(debug){
    String shttemp = String (data.temp);
    String shthum = String (data.humidity);
    Serial.println(dataAppender("Temp :" + shttemp + " Humidity : "+ shthum));
  }
}

unsigned int htdu21d_readTemp()
{

  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(TRIGGER_TEMP_MEASURE_NOHOLD);
  Wire.endTransmission();

  //Wait for sensor to complete measurement
  //delay(60); //44-50 ms max - we could also poll the sensor
  delay(95);

  //Comes back in three bytes, data(MSB) / data(LSB) / CRC
  Wire.requestFrom(HTDU21D_ADDRESS, 3);

  //Wait for data to become available
  int counter = 0;
  while(Wire.available() < 3)
  {
    counter++;
    delay(1);
    if(counter > 100) return 998; //Error out
  }

  unsigned char msb, lsb, crc;
  msb = Wire.read();
  lsb = Wire.read();
  crc = Wire.read(); //We don't do anything with CRC for now

  unsigned int temperature = ((unsigned int)msb << 8) | lsb;
  temperature &= 0xFFFC; //Zero out the status bits but keep them in place

  return temperature;
}

//Read the humidity
unsigned int htdu21d_readHumidity()
{
  byte msb, lsb, checksum;

  //Request a humidity reading
  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(TRIGGER_HUMD_MEASURE_NOHOLD); //Measure humidity with no bus holding
  Wire.endTransmission();

  //Hang out while measurement is taken. 50mS max, page 4 of datasheet.
  //delay(55);
  delay(95);
  //Read result
  Wire.requestFrom(HTDU21D_ADDRESS, 3);

  //Wait for data to become available
  int counter = 0;
  while(Wire.available() < 3)
  {
    counter++;
    delay(1);
    if(counter > 100) return 0; //Error out
  }

  msb = Wire.read();
  lsb = Wire.read();
  checksum = Wire.read();

  unsigned int rawHumidity = ((unsigned int) msb << 8) | (unsigned int) lsb;
  rawHumidity &= 0xFFFC; //Zero out the status bits but keep them in place

  return(rawHumidity);
}

//Given the raw temperature data, calculate the actual temperature
float calc_temp(int SigTemp)
{
  float tempSigTemp = SigTemp / (float)65536; //2^16 = 65536
  float realTemperature = -46.85 + (175.72 * tempSigTemp); //From page 14

  return(realTemperature);
}

//Given the raw humidity data, calculate the actual relative humidity
float calc_humidity(int SigRH)
{
  float tempSigRH = SigRH / (float)65536; //2^16 = 65536
  float rh = -6 + (125 * tempSigRH); //From page 14

  return(rh);
}

void restartESP() {
  server.send( 200, "text/json", "true");
  ESP.reset();
}

void handleRoot() {
  digitalWrite(LED_BUILTIN, 1);
  page = "<h2>Click <a href=\"/reset\">here</a> to reset esp</h2>";
  page += "<h2>Click <a href=\"/update\">here</a> to update esp</h2>";
  server.send(200, "text/html", page);
  digitalWrite(LED_BUILTIN, 0);
}

void handleNotFound(){
  digitalWrite(LED_BUILTIN, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(LED_BUILTIN, 0);
}

// void queryMDNSServices(){
//   Serial.println("Sending mDNS query");
//   int n = MDNS.queryService("esp", "tcp"); // Send out query for esp tcp services
//   Serial.println("mDNS query done");
//   if (n == 0) {
//     Serial.println("no services found");
//   }
//   else {
//     Serial.print(n);
//     Serial.println(" service(s) found");
//     for (int i = 0; i < n; ++i) {
//       // Print details for each service found
//       Serial.print(i + 1);
//       Serial.print(": ");
//       Serial.print(MDNS.hostname(i));
//       Serial.print(" (");
//       Serial.print(MDNS.IP(i));
//       Serial.print(":");
//       Serial.print(MDNS.port(i));
//       Serial.println(")");
//     }
//   }
// }

void startOTAService(){
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void startWifi(){
  // WiFi.mode(WIFI_STA);
  WiFi.mode(WIFI_AP_STA);
  WiFi.hostname(host); // this allows mDNS to funtion properly
  // WiFi.config(ip,gateway, subnet); // --> this is OPTIONAL -set desired IP. comment if working with MDNS

  //connect to a network
  WiFi.begin(ssid, password);


  Serial.print(dataAppender("**"));
  Serial.print(dataAppender("\nStarting up"));
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(dataAppender("\nConnected to " + (String)ssid));
  Serial.println(dataAppender("IP address: " +WiFi.localIP().toString()));
  // Serial.println(dataAppender(WiFi.localIP().toString()));

  //create a network
  WiFi.softAP("ESP", "00000000", 1, true); //for an open network without a password change to:  WiFi.softAP(ssid);

}
