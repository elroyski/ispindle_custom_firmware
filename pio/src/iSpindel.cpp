/**************************************************************

"iSpindel"
All rights reserverd by S.Lang <universam@web.de>

**************************************************************/

// includes go here
#include "Globals.h"
#include "MPUOffset.h"
#include <PubSubClient.h>
// #endif
#include "OneWire.h"
#include "Wire.h"
// #include <Ticker.h>
#include "DallasTemperature.h"
#include "DoubleResetDetector.h" // https://github.com/datacute/DoubleResetDetector
#include "RunningMedian.h"
#include "Sender.h"
#include "WiFiManagerKT.h"
#include "secrets.h" //AWS - Currently a file for Keys, Certs, etc - Need to make this a captured variable for iSpindle
#include "tinyexpr.h"
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <FS.h>          //this needs to be first
#include <LittleFS.h>
// !DEBUG 1

// Definicje zmiennych i stałych dla interfejsu użytkownika
const char HTTP_API_LIST[] = "<select id=\"selAPI\" onchange=\"document.getElementById('selAPI').value = document.getElementById('selAPI').options[document.getElementById('selAPI').selectedIndex].value\">"
"<option value=\"0\">Ubidots</option>"
"<option value=\"1\">CraftBeerPi</option>"
"<option value=\"2\">HTTP</option>"
"<option value=\"3\">TControl</option>"
"<option value=\"4\">FHEM</option>"
"<option value=\"5\">TCP</option>"
"<option value=\"6\">iSpindel.de</option>"
"<option value=\"7\">MQTT</option>"
"<option value=\"8\">Blynk</option>"
"<option value=\"9\">BrewPiLess</option>"
"<option value=\"10\" disabled>Thingspeak</option>"
"<option value=\"11\">InfluxDB 2</option>"
"<option value=\"12\">InfluxDB</option>"
"<option value=\"13\">Prometheus</option>"
"<option value=\"14\">Home Assistant</option>"
"<option value=\"15\">Webhook</option>"
"</select>";

const char HTTP_TEMPSCALE_LIST[] = "<select id=\"selTempscale\" onchange=\"document.getElementById('selTempscale').value = document.getElementById('selTempscale').options[document.getElementById('selTempscale').selectedIndex].value\">"
"<option value=\"0\">Celsius</option>"
"<option value=\"1\">Fahrenheit</option>"
"<option value=\"2\">Kelvin</option>"
"</select>";

#define TYPE_HIDDEN "type=\"hidden\""
#define TYPE_NUMBER "type=\"number\" min=\"0\" step=\"1\""
#define TYPE_CHECKBOX "type=\"checkbox\""
#define TYPE_CHECKBOX_CHECKED "type=\"checkbox\" checked=\"checked\""

// Globals
MPU6050 accelgyro;
OneWire *oneWire;
DallasTemperature DS18B20;
DeviceAddress tempDeviceAddress;
Ticker flasher;
RunningMedian samples = RunningMedian(MEDIANROUNDSMAX);
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

int detectTempSensor(const uint8_t pins[]);
bool testAccel();

bool shouldSaveConfig = false;

iData myData;

#if API_MQTT_HASSIO
//myData.hassio = false;
bool hassio_changed = false;
#endif
bool usehttps_changed = false;

uint32_t DSreqTime = 0;

int16_t ax, ay, az;
float Volt, Temperatur, Tilt, Gravity;

float scaleTemperatureFromC(float t, uint8_t tempscale)
{
  if (tempscale == TEMP_CELSIUS)
    return t;
  else if (tempscale == TEMP_FAHRENHEIT)
    return (1.8f * t + 32);
  else if (tempscale == TEMP_KELVIN)
    return t + 273.15f;
  else
    return t; // Invalid value for myData.tempscale => default to celsius
}

String tempScaleLabel(uint8_t tempscale)
{
  if (tempscale == TEMP_CELSIUS)
    return "C";
  else if (tempscale == TEMP_FAHRENHEIT)
    return "F";
  else if (tempscale == TEMP_KELVIN)
    return "K";
  else
    return "C"; // Invalid value for myData.tempscale => default to celsius
}

// callback notifying us of the need to save config
void saveConfigCallback()
{
  shouldSaveConfig = true;
}

void applyOffset()
{
  if (myData.Offset[0] != UNINIT && myData.Offset[1] != UNINIT && myData.Offset[2] != UNINIT)
  {
    CONSOLELN(String("applying offsets: ") + myData.Offset[0] + ":" + myData.Offset[1] + ":" + myData.Offset[2]);

    accelgyro.setXAccelOffset(myData.Offset[0]);
    accelgyro.setYAccelOffset(myData.Offset[1]);
    accelgyro.setZAccelOffset(myData.Offset[2]);
    accelgyro.setXGyroOffset(myData.Offset[3]);
    accelgyro.setYGyroOffset(myData.Offset[4]);
    accelgyro.setZGyroOffset(myData.Offset[5]);
    delay(1);

    CONSOLELN(String("confirming offsets: ") + accelgyro.getXAccelOffset() + ":" + accelgyro.getYAccelOffset() + ":" +
              accelgyro.getZAccelOffset());
  }
  else
    CONSOLELN(F("offsets not available"));
}

bool readConfig()
{
  CONSOLE(F("mounting FS..."));

  if (!LittleFS.begin())
  {
    CONSOLELN(F(" ERROR: failed to mount FS!"));
    return false;
  }
  else
  {
    CONSOLELN(F(" mounted!"));
    if (!LittleFS.exists(CFGFILE))
    {
      CONSOLELN(F("ERROR: failed to load json config"));
      return false;
    }
    else
    {
      // file exists, reading and loading
      CONSOLELN(F("reading config file"));
      File configFile = LittleFS.open(CFGFILE, "r");
      if (!configFile)
      {
        CONSOLELN(F("ERROR: unable to open config file"));
      }
      else
      {
        size_t size = configFile.size();
        DynamicJsonDocument doc(size * 3);
        DeserializationError error = deserializeJson(doc, configFile);
        if (error)
        {
          CONSOLE(F("deserializeJson() failed: "));
          CONSOLELN(error.c_str());
        }
        else
        {
          if (doc.containsKey("Name"))
            strcpy(myData.name, doc["Name"]);
          if (doc.containsKey("Token"))
            strcpy(myData.token, doc["Token"]);
          if (doc.containsKey("Server"))
            strcpy(myData.server, doc["Server"]);
          if (doc.containsKey("Sleep"))
            myData.sleeptime = doc["Sleep"];
          if (doc.containsKey("API"))
            myData.api = doc["API"];
          if (doc.containsKey("Port"))
            myData.port = doc["Port"];
          if (doc.containsKey("Channel"))
            myData.channel = doc["Channel"];
          if (doc.containsKey("URI"))
            strcpy(myData.uri, doc["URI"]);
          if (doc.containsKey("Username"))
            strcpy(myData.username, doc["Username"]);
          if (doc.containsKey("Password"))
            strcpy(myData.password, doc["Password"]);
          if (doc.containsKey("Job"))
            strcpy(myData.job, doc["Job"]);
          if (doc.containsKey("Instance"))
            strcpy(myData.instance, doc["Instance"]);
          if (doc.containsKey("Vfact"))
            myData.vfact = doc["Vfact"];
          if (doc.containsKey("TS"))
            myData.tempscale = doc["TS"];
          if (doc.containsKey("OWpin"))
            myData.OWpin = doc["OWpin"];
          if (doc.containsKey("SSID"))
            myData.ssid = (const char *)doc["SSID"];
          if (doc.containsKey("PSK"))
            myData.psk = (const char *)doc["PSK"];
          if (doc.containsKey("POLY"))
            strcpy(myData.polynominal, doc["POLY"]);
#if API_MQTT_HASSIO
          if (doc.containsKey("Hassio"))
            myData.hassio = doc["Hassio"];
#endif
          if (doc.containsKey("UseHTTPS"))
            myData.usehttps = doc["UseHTTPS"];
          if (doc.containsKey("Offset"))
          {
            for (size_t i = 0; i < (sizeof(myData.Offset) / sizeof(*myData.Offset)); i++)
            {
              myData.Offset[i] = doc["Offset"][i];
            }
          }

          CONSOLELN(F("parsed config:"));
#ifdef DEBUG
          serializeJson(doc, Serial);
          CONSOLELN();
#endif
        }
      }
    }
  }
  return true;
}

bool shouldStartConfig(bool validConf)
{

  // we make sure that configuration is properly set and we are not woken by
  // RESET button
  // ensure this was called

  rst_info *_reset_info = ESP.getResetInfoPtr();
  uint8_t _reset_reason = _reset_info->reason;

  // The ESP reset info is sill buggy. see http://www.esp8266.com/viewtopic.php?f=32&t=8411
  // The reset reason is "5" (woken from deep-sleep) in most cases (also after a power-cycle)
  // I added a single reset detection as workaround to enter the config-mode easier
  CONSOLE(F("Boot-Mode: "));
  CONSOLELN(ESP.getResetReason());
  bool _poweredOnOffOn = _reset_reason == REASON_DEFAULT_RST || _reset_reason == REASON_EXT_SYS_RST;
  if (_poweredOnOffOn)
    CONSOLELN(F("power-cycle or reset detected, config mode"));

  bool _dblreset = drd.detectDoubleReset();
  if (_dblreset)
    CONSOLELN(F("\nDouble Reset detected"));

  bool _wifiCred = (WiFi.SSID() != "");
  uint8_t c = 0;
  if (!_wifiCred)
  {
    if (strlen(myData.name) != 0)
      WiFi.hostname(myData.name); //Set DNS hostname

    WiFi.begin();
  }
  while (!_wifiCred)
  {
    if (c > 10)
      break;
    CONSOLE('.');
    delay(100);
    c++;
    _wifiCred = (WiFi.SSID() != "");
  }
  if (!_wifiCred)
    CONSOLELN(F("\nERROR no Wifi credentials"));

  if (validConf && !_dblreset && _wifiCred && !_poweredOnOffOn)
  {
    CONSOLELN(F("\nwoken from deepsleep, normal mode"));
    return false;
  }
  // config mode
  else
  {
    CONSOLELN(F("\ngoing to Config Mode"));
    return true;
  }
}

void validateInput(const char *input, char *output)
{
  String tmp = input;
  tmp.trim();
  tmp.replace(' ', '_');
  tmp.toCharArray(output, tmp.length() + 1);
}

String htmlencode(String str)
{
  String encodedstr = "";
  char c;

  for (uint16_t i = 0; i < str.length(); i++)
  {
    c = str.charAt(i);

    if (isalnum(c))
    {
      encodedstr += c;
    }
    else
    {
      encodedstr += "&#";
      encodedstr += String((uint8_t)c);
      encodedstr += ';';
    }
  }
  return encodedstr;
}

void postConfig()
{
#if API_MQTT_HASSIO
  SenderClass sender;
  if (myData.hassio)
  {
    sender.enableHassioDiscovery(myData.server, myData.port, myData.username, myData.password, myData.name,
                                 tempScaleLabel(myData.tempscale));
  }
  if (hassio_changed && !myData.hassio)
  {
    sender.disableHassioDiscovery(myData.server, myData.port, myData.username, myData.password, myData.name);
  }
#endif
}

bool startConfiguration()
{

  WiFiManager wifiManager;

  wifiManager.setConfigPortalTimeout(PORTALTIMEOUT);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setBreakAfterConfig(true);

  WiFiManagerParameter api_list(HTTP_API_LIST);
  WiFiManagerParameter custom_api("selAPI", "selAPI", String(myData.api).c_str(), 20, TYPE_HIDDEN, WFM_NO_LABEL);

  WiFiManagerParameter custom_name("name", "iSpindel Name", htmlencode(myData.name).c_str(), TKIDSIZE);
  WiFiManagerParameter custom_sleep("sleep", "Update Interval (s)", String(myData.sleeptime).c_str(), 6, TYPE_NUMBER);
  WiFiManagerParameter custom_token("token", "Token/ API key", htmlencode(myData.token).c_str(), TKIDSIZE * 2);
  WiFiManagerParameter custom_server("server", "Server Address", myData.server, DNSSIZE);
  WiFiManagerParameter custom_port("port", "Server Port", String(myData.port).c_str(), TKIDSIZE, TYPE_NUMBER);
  WiFiManagerParameter custom_channel("channel", "Channelnumber", String(myData.channel).c_str(), TKIDSIZE,
                                      TYPE_NUMBER);
  WiFiManagerParameter custom_uri("uri", "Path / URI", myData.uri, DNSSIZE);
  WiFiManagerParameter custom_username("username", "Username", myData.username, TKIDSIZE);
  WiFiManagerParameter custom_password("password", "Password", myData.password, DNSSIZE);
  WiFiManagerParameter custom_job("job", "Prometheus job", myData.job, TKIDSIZE);
  WiFiManagerParameter custom_instance("instance", "Prometheus instance", myData.instance, TKIDSIZE);
#if API_MQTT_HASSIO
  WiFiManagerParameter custom_hassio("hassio", "Home Assistant integration via MQTT", "checked", TKIDSIZE,
                                     myData.hassio ? TYPE_CHECKBOX_CHECKED : TYPE_CHECKBOX);
#endif
  WiFiManagerParameter custom_usehttps("usehttps", "Connect to server via HTTPS", "checked", TKIDSIZE,
                                       myData.usehttps ? TYPE_CHECKBOX_CHECKED : TYPE_CHECKBOX);
  WiFiManagerParameter custom_vfact("vfact", "Battery conversion factor", String(myData.vfact).c_str(), 7, TYPE_NUMBER);
  WiFiManagerParameter tempscale_list(HTTP_TEMPSCALE_LIST);
  WiFiManagerParameter custom_tempscale("tempscale", "tempscale", String(myData.tempscale).c_str(), 5, TYPE_HIDDEN,
                                        WFM_NO_LABEL);
  WiFiManagerParameter custom_warning1(
      "warning1",
      "WARNING! Secure MQTT has a big impact on battery usage.<BR>&nbsp;<BR>For AWS:<UL><LI>Name must be "
      "Thingname</LI><LI>Server must be Endpoint</LI><LI>Port must be 8883</LI><LI>Path/URI is Publish Topic</LI></UL>",
      "<<<<< >>>>>", TKIDSIZE);

  wifiManager.addParameter(&custom_name);
  wifiManager.addParameter(&custom_sleep);
  wifiManager.addParameter(&custom_vfact);

  WiFiManagerParameter custom_tempscale_hint("<label for=\"TS\">Unit of temperature</label>");
  wifiManager.addParameter(&custom_tempscale_hint);
  wifiManager.addParameter(&tempscale_list);
  wifiManager.addParameter(&custom_tempscale);
  WiFiManagerParameter custom_api_hint("<hr><label for=\"API\">Service Type</label>");
  wifiManager.addParameter(&custom_api_hint);

  wifiManager.addParameter(&api_list);
  wifiManager.addParameter(&custom_api);

  wifiManager.addParameter(&custom_warning1);
  wifiManager.addParameter(&custom_token);
  wifiManager.addParameter(&custom_server);
  wifiManager.addParameter(&custom_port);
  wifiManager.addParameter(&custom_channel);
  wifiManager.addParameter(&custom_uri);
  wifiManager.addParameter(&custom_username);
  wifiManager.addParameter(&custom_password);
  wifiManager.addParameter(&custom_job);
  wifiManager.addParameter(&custom_instance);
#if API_MQTT_HASSIO
  wifiManager.addParameter(&custom_hassio);
#endif
  wifiManager.addParameter(&custom_usehttps);
  WiFiManagerParameter custom_polynom_lbl(
      "<hr><label for=\"POLYN\">Gravity conversion<br/>ex. \"-0.00031*tilt^2+0.557*tilt-14.054\"</label>");
  wifiManager.addParameter(&custom_polynom_lbl);
  WiFiManagerParameter custom_polynom("POLYN", "Polynominal", htmlencode(myData.polynominal).c_str(), 250,
                                      WFM_NO_LABEL);
  wifiManager.addParameter(&custom_polynom);

  wifiManager.setConfSSID(htmlencode(myData.ssid));
  wifiManager.setConfPSK(htmlencode(myData.psk));

  CONSOLELN(F("started Portal"));
  static char ssid[33] = {0}; //32 char max for SSIDs
  if (strlen(myData.name) == 0)
    snprintf(ssid, sizeof ssid, "iSpindel_%06X", ESP.getChipId());
  else
  {
    snprintf(ssid, sizeof ssid, "iSpindel_%s", myData.name);
    WiFi.hostname(myData.name); //Set DNS hostname
  }

  wifiManager.startConfigPortal(ssid);

  strcpy(myData.polynominal, custom_polynom.getValue());

  validateInput(custom_name.getValue(), myData.name);
  validateInput(custom_token.getValue(), myData.token);
  validateInput(custom_server.getValue(), myData.server);
  validateInput(custom_username.getValue(), myData.username);
  validateInput(custom_password.getValue(), myData.password);
  validateInput(custom_job.getValue(), myData.job);
  validateInput(custom_instance.getValue(), myData.instance);
  myData.sleeptime = String(custom_sleep.getValue()).toInt();

  myData.api = String(custom_api.getValue()).toInt();
  myData.port = String(custom_port.getValue()).toInt();
  myData.channel = String(custom_channel.getValue()).toInt();
  myData.tempscale = String(custom_tempscale.getValue()).toInt();
#if API_MQTT_HASSIO
  {
    auto hassio = myData.api == DTMQTT && String(custom_hassio.getValue()) == "checked";
    hassio_changed = myData.hassio != hassio;
    myData.hassio = hassio;
  }
#endif
  {
    auto usehttps = myData.api == DTInfluxDB && String(custom_usehttps.getValue()) == "checked";
    usehttps_changed = myData.usehttps != usehttps;
    myData.usehttps = usehttps;
  }
  validateInput(custom_uri.getValue(), myData.uri);

  String tmp = custom_vfact.getValue();
  tmp.trim();
  tmp.replace(',', '.');
  myData.vfact = tmp.toFloat();
  if (myData.vfact < ADCDIVISOR * 0.8 || myData.vfact > ADCDIVISOR * 1.25)
    myData.vfact = ADCDIVISOR;

  // save the custom parameters to FS
  if (shouldSaveConfig)
  {
    // Wifi config
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);

    postConfig();

    return saveConfig();
  }
  return false;
}

bool formatLittleFS()
{
  CONSOLE(F("\nneed to format LittleFS: "));
  LittleFS.end();
  LittleFS.begin();
  CONSOLELN(LittleFS.format());
  return LittleFS.begin();
}

bool saveConfig(int16_t Offset[6])
{
  std::copy(Offset, Offset + 6, myData.Offset);
  CONSOLELN(String("new offsets: ") + Offset[0] + ":" + Offset[1] + ":" + Offset[2]);
  CONSOLELN(String("confirming offsets: ") + accelgyro.getXAccelOffset() + ":" + accelgyro.getYAccelOffset() + ":" +
            accelgyro.getZAccelOffset());

  return saveConfig();
}

float calibrateToVref(float measuredVref)
{
  analogRead(A0);
  float A0value = analogRead(A0);
  float factor = A0value / measuredVref;
  CONSOLELN(String("A0: ") + A0value);
  CONSOLELN(String("Vref: ") + measuredVref);
  CONSOLELN(String("Factor: ") + factor);
  myData.vfact = factor;
  saveConfig();
  return factor;
}

bool saveConfig()
{
  CONSOLE(F("saving config...\n"));

  // if LittleFS is not usable
  if (!LittleFS.begin())
  {
    Serial.println("Failed to mount file system");
    if (!formatLittleFS())
    {
      Serial.println("Failed to format file system - hardware issues!");
      return false;
    }
  }

  DynamicJsonDocument doc(2048);

  doc["Name"] = myData.name;
  doc["Token"] = myData.token;
  doc["Sleep"] = myData.sleeptime;
  // first reboot is for test
  myData.sleeptime = 1;
  doc["Server"] = myData.server;
  doc["API"] = myData.api;
  doc["Port"] = myData.port;
  doc["Channel"] = myData.channel;
  doc["URI"] = myData.uri;
  doc["Username"] = myData.username;
  doc["Password"] = myData.password;
  doc["Job"] = myData.job;
  doc["Instance"] = myData.instance;
#if API_MQTT_HASSIO
  doc["Hassio"] = myData.hassio;
#endif
  doc["UseHTTPS"] = myData.usehttps;
  doc["Vfact"] = myData.vfact;
  doc["TS"] = myData.tempscale;
  doc["OWpin"] = myData.OWpin;
  doc["POLY"] = myData.polynominal;
  doc["SSID"] = WiFi.SSID();
  doc["PSK"] = WiFi.psk();

  JsonArray array = doc.createNestedArray("Offset");
  for (auto &&i : myData.Offset)
  {
    array.add(i);
  }

  File configFile = LittleFS.open(CFGFILE, "w");
  if (!configFile)
  {
    CONSOLELN(F("failed to open config file for writing"));
    LittleFS.end();
    return false;
  }
  else
  {
    serializeJson(doc, configFile);
#ifdef DEBUG
    serializeJson(doc, Serial);
#endif
    configFile.flush();
    configFile.close();
    LittleFS.gc();
    LittleFS.end();
    CONSOLELN(F("\nsaved successfully"));
    return true;
  }
}

bool processResponse(String response)
{
  DynamicJsonDocument doc(1024);

  DeserializationError error = deserializeJson(doc, response);
  if (!error && doc.containsKey("interval"))
  {
    uint32_t interval = doc["interval"];
    if (interval != myData.sleeptime && interval < 24 * 60 * 60 && interval > 10)
    {
      myData.sleeptime = interval;
      CONSOLE(F("Received new Interval config: "));
      CONSOLELN(interval);
      return saveConfig();
    }
  }
  return false;
}

bool uploadData(uint8_t service)
{
  SenderClass sender;

#if API_UBIDOTS
  if (service == DTUbiDots)
  {
    sender.add("tilt", Tilt);
    sender.add("temperature", scaleTemperatureFromC(Temperatur, myData.tempscale));
    sender.add("battery", Volt);
    sender.add("gravity", Gravity);
    sender.add("interval", myData.sleeptime);
    sender.add("RSSI", WiFi.RSSI());
    CONSOLELN(F("\ncalling Ubidots"));
    return sender.sendUbidots(myData.token, myData.name);
  }
#endif

#if API_AWSIOTMQTT //AWS
  if (service == DTAWSIOTMQTT)
  {
    sender.add("name", myData.name);
    sender.add("tilt", Tilt);
    sender.add("temperature", scaleTemperatureFromC(Temperatur, myData.tempscale));
    sender.add("battery", Volt);
    sender.add("gravity", Gravity);
    sender.add("interval", myData.sleeptime);
    sender.add("RSSI", WiFi.RSSI());
    CONSOLELN("Calling AWSIOTMQTT Sender");
    return sender.sendSecureMQTT(AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, myData.server, myData.port, myData.name,
                                 myData.uri);
    //AWS - NOTE - Need to replace secrets.h with the relevant parameters
  }
#endif

#if API_MQTT
  if (service == DTMQTT)
  {
    sender.add("tilt", Tilt);
    sender.add("temperature", scaleTemperatureFromC(Temperatur, myData.tempscale));
    sender.add("temp_units", tempScaleLabel(myData.tempscale));
    sender.add("battery", Volt);
    sender.add("gravity", Gravity);
    sender.add("interval", myData.sleeptime);
    sender.add("RSSI", WiFi.RSSI());
    CONSOLELN(F("\ncalling MQTT"));
    return sender.sendMQTT(myData.server, myData.port, myData.username, myData.password, myData.name);
  }
#endif

#if API_THINGSPEAK
  if (service == DTTHINGSPEAK)
  {
    sender.add("tilt", Tilt);
    sender.add("temperature", scaleTemperatureFromC(Temperatur, myData.tempscale));
    sender.add("temp_units", tempScaleLabel(myData.tempscale));
    sender.add("battery", Volt);
    sender.add("gravity", Gravity);
    sender.add("interval", myData.sleeptime);
    sender.add("RSSI", WiFi.RSSI());
    CONSOLELN(F("\ncalling ThingSpeak"));
    return sender.sendThingSpeak(myData.token, myData.channel);
  }
#endif

#if API_INFLUXDB
  if (service == DTInfluxDB)
  {
    sender.add("tilt", Tilt);
    sender.add("temperature", scaleTemperatureFromC(Temperatur, myData.tempscale));
    sender.add("temp_units", tempScaleLabel(myData.tempscale));
    sender.add("battery", Volt);
    sender.add("gravity", Gravity);
    sender.add("interval", myData.sleeptime);
    sender.add("RSSI", WiFi.RSSI());
    CONSOLELN(F("\ncalling InfluxDB"));

    return sender.sendInfluxDB(myData.server, myData.port, myData.uri, myData.name, myData.username, myData.password,
                               myData.usehttps);
  }
#endif

#if API_PROMETHEUS
  if (service == DTPrometheus)
  {
    sender.add("tilt", Tilt);
    sender.add("temperature", Temperatur);
    sender.add("battery", Volt);
    sender.add("gravity", Gravity);
    sender.add("interval", myData.sleeptime);
    sender.add("RSSI", WiFi.RSSI());
    CONSOLELN(F("\ncalling Prometheus Pushgateway"));
    return sender.sendPrometheus(myData.server, myData.port, myData.job, myData.instance);
  }
#endif

#if API_GENERIC
  if ((service == DTHTTP) || (service == DTCraftBeerPi) || (service == DTiSPINDELde) || (service == DTTCP) ||
      (service == DTHTTPS))
  {

    sender.add("name", myData.name);
    sender.add("ID", ESP.getChipId());
    if (myData.token[0] != 0)
      sender.add("token", myData.token);
    sender.add("angle", Tilt);
    sender.add("temperature", scaleTemperatureFromC(Temperatur, myData.tempscale));
    sender.add("temp_units", tempScaleLabel(myData.tempscale));
    sender.add("battery", Volt);
    sender.add("gravity", Gravity);
    sender.add("interval", myData.sleeptime);
    sender.add("RSSI", WiFi.RSSI());

    if (service == DTHTTP)
    {
      CONSOLELN(F("\ncalling HTTP"));
      return sender.sendGenericPost(myData.server, myData.uri, myData.port);
    }
    else if (service == DTCraftBeerPi)
    {
      CONSOLELN(F("\ncalling CraftbeerPi"));
      return sender.sendGenericPost(myData.server, CBP_ENDPOINT, 5000);
    }
    else if (service == DTiSPINDELde)
    {
      CONSOLELN(F("\ncalling iSPINDELde"));
      return sender.sendTCP("ispindle.de", 9501).length() > 0;
    }
    else if (service == DTTCP)
    {
      CONSOLELN(F("\ncalling TCP"));
      String response = sender.sendTCP(myData.server, myData.port);
      return processResponse(response);
    }
    else if (service == DTHTTPS)
    {
      CONSOLELN(F("\ncalling HTTPS"));
      return sender.sendHTTPSPost(myData.server, myData.uri);
    }
  }
#endif // DATABASESYSTEM

#if API_FHEM
  if (service == DTFHEM)
  {
    sender.add("angle", Tilt);
    sender.add("temperature", scaleTemperatureFromC(Temperatur, myData.tempscale));
    sender.add("temp_units", tempScaleLabel(myData.tempscale));
    sender.add("battery", Volt);
    sender.add("gravity", Gravity);
    sender.add("ID", ESP.getChipId());
    CONSOLELN(F("\ncalling FHEM"));
    return sender.sendFHEM(myData.server, myData.port, myData.name);
  }
#endif // DATABASESYSTEM ==
#if API_TCONTROL
  if (service == DTTcontrol)
  {
    sender.add("T", scaleTemperatureFromC(Temperatur, myData.tempscale));
    sender.add("D", Tilt);
    sender.add("U", Volt);
    sender.add("G", Gravity);
    CONSOLELN(F("\ncalling TCONTROL"));
    return sender.sendTCONTROL(myData.server, 4968);
  }
#endif // DATABASESYSTEM ==

#if API_BLYNK
  if (service == DTBLYNK)
  {
    String tempToSend = String(scaleTemperatureFromC(Temperatur, myData.tempscale), 1);
    sender.add("20", tempToSend); //send temperature without the unit to the graph first
    String voltToSend = String(Volt, 2);
    sender.add("30", voltToSend); //send temperature without the unit to the graph first

    tempToSend += "°";
    tempToSend += tempScaleLabel(myData.tempscale); // Add temperature unit to the String

    sender.add("1", String(Tilt, 1) + "°");
    sender.add("2", tempToSend);
    sender.add("3", voltToSend + "V");
    sender.add("4", String(Gravity, 3));
    return sender.sendBlynk(myData.token);
  }
#endif

#if API_BREWBLOX
  if (service == DTBREWBLOX)
  {
    sender.add("Tilt[deg]", Tilt);
    sender.add("Temperature[deg" + tempScaleLabel(myData.tempscale) + "]",
               scaleTemperatureFromC(Temperatur, myData.tempscale));
    sender.add("Battery[V]", Volt);
    sender.add("Gravity", Gravity);
    sender.add("Rssi[dBm]", WiFi.RSSI());
    CONSOLELN(F("\ncalling BREWBLOX"));
    return sender.sendBrewblox(myData.server, myData.port, myData.uri, myData.username, myData.password, myData.name);
  }
#endif

#ifdef API_BRICKS
  if (service == DTBRICKS)
  {
    CONSOLELN(F("adding BRICKS params"));

    if (myData.token[0] != 0)
    {
      CONSOLELN(F("found token"));
      sender.add("apikey", myData.token); // use the token field as vessel for the api key
    }
    else
    {
      CONSOLELN(F("missing token in params"));
    }

    CONSOLELN(F("adding payload..."));
    String chipid = String(ESP.getFlashChipId()) + "_" + String(WiFi.macAddress());
    String chipidHashed = sender.createMd5Hash(chipid).substring(0, 16);
    sender.add("type", "ispindel");
    sender.add("brand", "wemos_d1_mini");
    sender.add("version", FIRMWAREVERSION);
    sender.add("chipid", chipidHashed);
    sender.add("s_number_wort_0", Gravity);    // gravity can be in SG or °P, depending on user setting
    sender.add("s_number_temp_0", Temperatur); // always transmit °C
    sender.add("s_number_voltage_0", Volt);
    sender.add("s_number_wifi_0", WiFi.RSSI());
    sender.add("s_number_tilt_0", Tilt);

    CONSOLELN(F("\ncalling BRICKS"));

    uint32_t sleeptime_candidate_s = sender.sendBricks() / 1000;

    if (sleeptime_candidate_s > 0)
    {
      myData.sleeptime = sleeptime_candidate_s;
      return true;
    }
    else
    {
      return false;
    }
  }
#endif
  return false;
}

void goodNight(uint32_t seconds)
{

  uint32_t _seconds = seconds;
  uint32_t left2sleep = 0;
  uint32_t validflag = RTCVALIDFLAG;

  drd.stop();

  // workaround for DS not floating
  pinMode(myData.OWpin, OUTPUT);
  digitalWrite(myData.OWpin, LOW);

  // we need another incarnation before work run
  if (_seconds > MAXSLEEPTIME)
  {
    left2sleep = _seconds - MAXSLEEPTIME;
    ESP.rtcUserMemoryWrite(RTCSLEEPADDR, &left2sleep, sizeof(left2sleep));
    ESP.rtcUserMemoryWrite(RTCSLEEPADDR + 1, &validflag, sizeof(validflag));
    CONSOLELN(String(F("\nStep-sleep: ")) + MAXSLEEPTIME + "s; left: " + left2sleep + "s; RT: " + millis());
    ESP.deepSleep(MAXSLEEPTIME * 1e6, WAKE_RF_DISABLED);
  }
  // regular sleep with RF enabled after wakeup
  else
  {
    // clearing RTC to mark next wake
    left2sleep = 0;
    ESP.rtcUserMemoryWrite(RTCSLEEPADDR, &left2sleep, sizeof(left2sleep));
    ESP.rtcUserMemoryWrite(RTCSLEEPADDR + 1, &validflag, sizeof(validflag));
    CONSOLELN(String(F("\nFinal-sleep: ")) + _seconds + "s; RT: " + millis());
    // WAKE_RF_DEFAULT --> auto reconnect after wakeup
    ESP.deepSleep(_seconds * 1e6, WAKE_RF_DEFAULT);
  }
  // workaround proper power state init
  delay(500);
}
void sleepManager()
{
  uint32_t left2sleep, validflag;
  ESP.rtcUserMemoryRead(RTCSLEEPADDR, &left2sleep, sizeof(left2sleep));
  ESP.rtcUserMemoryRead(RTCSLEEPADDR + 1, &validflag, sizeof(validflag));

  // check if we have to incarnate again
  if (left2sleep != 0 && !drd.detectDoubleReset() && validflag == RTCVALIDFLAG)
  {
    goodNight(left2sleep);
  }
  else
  {
    CONSOLELN(F("Worker run!"));
  }
}

void requestTemp()
{
  if (!DSreqTime)
  {
    DS18B20.requestTemperatures();
    DSreqTime = millis();
  }
}

void initDS18B20()
{
  CONSOLE(F("Diagnostyka: "));
  CONSOLE(F("Wolna pamięć: "));
  CONSOLELN(ESP.getFreeHeap());
  CONSOLE(F("Uptime (ms): "));
  CONSOLELN(millis());
  
  CONSOLE(F("Inicjalizacja DS18B20... "));
  
  if (myData.OWpin == -1)
  {
    CONSOLELN(F("Nieznany pin, szukam czujnika..."));
    myData.OWpin = detectTempSensor(OW_PINS);
    if (myData.OWpin == -1)
    {
      CONSOLELN(F("ERROR: cannot find a OneWire Temperature Sensor!"));
      return;
    }
    CONSOLE(F("Znaleziono czujnik na pinie: "));
    CONSOLELN(myData.OWpin);
  }
  
  CONSOLE(F("Konfiguracja DS18B20 na pinie: "));
  CONSOLELN(myData.OWpin);
  
  // Sprawdzenie stanu pinu przed inicjalizacją
  pinMode(myData.OWpin, INPUT);
  CONSOLE(F("Stan pinu przed reset: "));
  CONSOLELN(digitalRead(myData.OWpin));
  
  pinMode(myData.OWpin, OUTPUT);
  digitalWrite(myData.OWpin, LOW);
  delay(100);
  
  CONSOLE(F("Tworzenie instancji OneWire... "));
  if (oneWire != NULL) {
    CONSOLELN(F("Poprzednia instancja OneWire istnieje - usuwam"));
    delete oneWire;
    oneWire = NULL;
  }
  
  oneWire = new OneWire(myData.OWpin);
  
  if (oneWire == NULL) {
    CONSOLELN(F("BŁĄD: Nie udało się zaalokować pamięci dla OneWire!"));
    CONSOLE(F("Wolna pamięć: "));
    CONSOLELN(ESP.getFreeHeap());
    return;
  } else {
    CONSOLELN(F("OK"));
  }
  
  DS18B20 = DallasTemperature(oneWire);
  DS18B20.begin();

  bool device = DS18B20.getAddress(tempDeviceAddress, 0);
  
  if (!device)
  {
    CONSOLELN(F("Nie wykryto urządzenia po adresie, próbuję ponownie wykryć czujnik..."));
    myData.OWpin = detectTempSensor(OW_PINS);
    if (myData.OWpin == -1)
    {
      CONSOLELN(F("ERROR: cannot find a OneWire Temperature Sensor!"));
      return;
    }
    else
    {
      CONSOLE(F("Znaleziono czujnik na innym pinie: "));
      CONSOLELN(myData.OWpin);
      pinMode(myData.OWpin, OUTPUT);
      digitalWrite(myData.OWpin, LOW);
      delay(100);
      
      if (oneWire != NULL) {
        delete oneWire;
        oneWire = NULL;
      }
      
      oneWire = new OneWire(myData.OWpin);
      DS18B20 = DallasTemperature(oneWire);
      DS18B20.begin();
      device = DS18B20.getAddress(tempDeviceAddress, 0);
      
      if (!device) {
        CONSOLELN(F("BŁĄD: Nadal nie mogę wykryć adresu czujnika DS18B20!"));
        CONSOLELN(F("Sprawdź podłączenie fizyczne lub wymień czujnik"));
      } else {
        CONSOLELN(F("Adres czujnika DS18B20 znaleziony po ponownej próbie"));
      }
    }
  }
  else {
    CONSOLELN(F("Znaleziono adres czujnika DS18B20"));
    CONSOLE(F("Adres urządzenia: "));
    for (uint8_t i = 0; i < 8; i++) {
      CONSOLE(tempDeviceAddress[i], HEX);
      CONSOLE(F(" "));
    }
    CONSOLELN();
  }

  DS18B20.setWaitForConversion(false);
  DS18B20.setResolution(tempDeviceAddress, RESOLUTION);
  CONSOLE(F("Ustawiona rozdzielczość: "));
  CONSOLELN(RESOLUTION);
  
  // Spróbuj pierwszy odczyt
  requestTemp();
  delay(OWinterval);
  float firstTemp = DS18B20.getTempCByIndex(0);
  CONSOLE(F("Pierwszy odczyt temperatury: "));
  if (firstTemp == DEVICE_DISCONNECTED_C || firstTemp == 85.0) {
    CONSOLELN(F("BŁĄD - nie udało się odczytać temperatury!"));
  } else {
    CONSOLE(firstTemp);
    CONSOLELN(F(" °C - OK"));
  }
  
  CONSOLE(F("Wolna pamięć po inicjalizacji DS18B20: "));
  CONSOLELN(ESP.getFreeHeap());
}

bool isDS18B20ready()
{
  return millis() - DSreqTime > OWinterval;
}

void initAccel()
{
  // join I2C bus (I2Cdev library doesn't do this automatically)
  Wire.begin(D3, D4);
  Wire.setClock(100000);
  Wire.setClockStretchLimit(2 * 230);

  testAccel();
  // init the Accel
  accelgyro.initialize();
  accelgyro.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
  accelgyro.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
  accelgyro.setDLPFMode(MPU6050_DLPF_BW_5);
  accelgyro.setTempSensorEnabled(true);
#ifdef USE_DMP
  accelgyro.setDMPEnabled(true);
  packetSize = accelgyro.dmpGetFIFOPacketSize();
#endif
  accelgyro.setInterruptLatch(0); // pulse
  accelgyro.setInterruptMode(1);  // Active Low
  accelgyro.setInterruptDrive(1); // Open drain
  accelgyro.setRate(17);
  accelgyro.setIntDataReadyEnabled(true);
  applyOffset();
}

float calculateTilt()
{
  if (ax == 0 && ay == 0 && az == 0)
    return 0.f;

  return acos(abs(az) / (sqrt(ax * ax + ay * ay + az * az))) * 180.0 / M_PI;
}

bool testAccel()
{
  uint8_t res = Wire.status();
  if (res != I2C_OK)
    CONSOLELN(String(F("I2C ERROR: ")) + res);

  bool con = false;
  auto id = accelgyro.getDeviceID();
  if (id == 0x34 || id == 0x38) //0x34 = MPU6050 | 0x38 = MPU6500
    con = true;

  if (!con)
    CONSOLELN(F("Acc Test Connection ERROR!"));

  return res == I2C_OK && con == true;
}

void getAccSample()
{
  accelgyro.getAcceleration(&ax, &az, &ay);
}

float getTilt()
{
  uint32_t start = millis();
  uint8_t i = 0;

  for (; i < MEDIANROUNDSMAX; i++)
  {
    uint8_t wait = 0;
    while (!accelgyro.getIntDataReadyStatus())
    {
      delay(2);

      if (++wait > 50)
      {
        CONSOLELN(F("Error: timed out waiting for gyro!"));
        break;
      }
    }

    getAccSample();
    float _tilt = calculateTilt();
    samples.add(_tilt);

    if (i >= MEDIANROUNDSMIN && isDS18B20ready())
      break;
  }
  CONSOLE("Samples:");
  CONSOLE(++i);
  CONSOLE(" min:");
  CONSOLE(samples.getLowest());
  CONSOLE(" max:");
  CONSOLE(samples.getHighest());
  CONSOLE(" time:");
  CONSOLELN(millis() - start);
  return samples.getAverage(MEDIANAVRG);
}

float getTemperature(bool block = false)
{
  float t = Temperatur;

  // we need to wait for DS18b20 to finish conversion
  if (!DSreqTime || (!block && !isDS18B20ready()))
    return t;

  // if we need the result we have to block
  while (!isDS18B20ready())
    delay(10);
  DSreqTime = 0;

  // Diagnostyka przed odczytem
  CONSOLE(F("getTemperature - Wolna pamięć: "));
  CONSOLELN(ESP.getFreeHeap());
  CONSOLE(F("Stan magistrali OneWire: "));
  
  if (oneWire == NULL) {
    CONSOLELN(F("ERROR: oneWire jest NULL!"));
    return t;
  }
  
  // Sprawdź stan magistrali
  bool resetStatus = oneWire->reset();
  CONSOLE(F("Reset: "));
  CONSOLELN(resetStatus ? "OK" : "BŁĄD");
  
  t = DS18B20.getTempCByIndex(0);

  if (t == DEVICE_DISCONNECTED_C || // DISCONNECTED
      t == 85.0)                    // we read 85 uninitialized
  {
    CONSOLE(F("ERROR: OW DISCONNECTED - Wartość odczytu: "));
    CONSOLELN(t);
    CONSOLE(F("Pin czujnika: "));
    CONSOLELN(myData.OWpin);
    
    // Sprawdź stan pinu
    pinMode(myData.OWpin, INPUT);
    CONSOLE(F("Stan pinu po błędzie: "));
    CONSOLELN(digitalRead(myData.OWpin));
    
    // Spróbujmy odczytać temperaturę z MPU6050 jako alternatywę
    float accTemp = accelgyro.getTemperature() / 340.00 + 36.53;
    CONSOLE(F("Temperatura z MPU6050: "));
    CONSOLELN(accTemp);
    
    pinMode(myData.OWpin, OUTPUT);
    digitalWrite(myData.OWpin, LOW);
    delay(100);
    oneWire->reset();
    
    CONSOLELN(F("OW Retry - Próba ponownej inicjalizacji DS18B20"));
    initDS18B20();
    delay(OWinterval);
    t = DS18B20.getTempCByIndex(0);
    
    // Sprawdź czy udało się odczytać temperaturę po ponownej inicjalizacji
    if (t == DEVICE_DISCONNECTED_C || t == 85.0) {
      CONSOLELN(F("Ponowna próba nieudana, używam temperatury z MPU6050"));
      t = accTemp;
    } else {
      CONSOLE(F("Udało się odczytać temperaturę z DS18B20 po ponownej próbie: "));
      CONSOLELN(t);
    }
  }

  return t;
}

int detectTempSensor(const uint8_t pins[])
{

  for (uint8_t p = 0; p < sizeof(pins); p++)
  {
    const byte pin = pins[p];
    byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];
    float celsius;

    CONSOLE(F("scanning for OW device on pin: "));
    CONSOLELN(pin);
    OneWire ds(pin);

    if (!ds.search(addr))
    {
      CONSOLELN(F("No devices found!"));
      ds.reset_search();
      delay(250);
      continue;
    }

    CONSOLE("Found device with ROM =");
    for (i = 0; i < 8; i++)
    {
      CONSOLE(' ');
      CONSOLE(addr[i], HEX);
    }

    if (OneWire::crc8(addr, 7) != addr[7])
    {
      CONSOLELN(" CRC is not valid!");
      continue;
    }
    CONSOLELN();

    // the first ROM byte indicates which chip
    switch (addr[0])
    {
    case 0x10:
      CONSOLELN("  Chip = DS18S20"); // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      CONSOLELN("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      CONSOLELN("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      CONSOLELN("Device is not a DS18x20 family device.");
      continue;
    }

    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1); // start conversion, with parasite power on at the end

    delay(900); // maybe 750ms is enough, maybe not
    present = ds.reset();
    ds.select(addr);
    ds.write(0xBE); // Read Scratchpad

    CONSOLE("  Data = ");
    CONSOLE(present, HEX);
    CONSOLE(" ");
    for (i = 0; i < 9; i++)
    { // we need 9 bytes
      data[i] = ds.read();
      CONSOLE(data[i], HEX);
      CONSOLE(" ");
    }
    CONSOLE(" CRC=");
    CONSOLELN(OneWire::crc8(data, 8), HEX);

    // Convert the data to actual temperature
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s)
    {
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10)
      {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
    }
    else
    {
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00)
        raw = raw & ~7; // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20)
        raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40)
        raw = raw & ~1; // 11 bit res, 375 ms
      //// default is 12 bit resolution, 750 ms conversion time
    }
    celsius = (float)raw / 16.0;
    CONSOLE(F("  Temperature = "));
    CONSOLE(celsius);
    CONSOLELN(F(" Celsius, "));
    return pin;
  }
  return -1;
}

float getBattery()
{
  analogRead(A0); // drop first read
  return analogRead(A0) / myData.vfact;
}

float calculateGravity()
{
  double _tilt = Tilt;
  double _temp = Temperatur;
  float _gravity = 0;
  int err;
  te_variable vars[] = {{"tilt", &_tilt}, {"temp", &_temp}};
  te_expr *expr = te_compile(myData.polynominal, vars, 2, &err);

  if (expr)
  {
    _gravity = te_eval(expr);
    te_free(expr);
  }
  else
  {
    CONSOLELN(String(F("Parse error at ")) + err);
  }
  return _gravity;
}

void flash()
{
  // triggers the LED
  Volt = getBattery();
  if (testAccel())
    getAccSample();
  Tilt = calculateTilt();
  Temperatur = getTemperature(false);
  Gravity = calculateGravity();
  requestTemp();
}

bool isSafeMode(float _volt)
{
  if (_volt < LOWBATT)
  {
    CONSOLELN(F("\nWARNING: low Battery"));
    return true;
  }
  else
    return false;
}

bool connectBackupCredentials()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); //suggestion that MQTT connection failures can happen if WIFI mode isn't STA.
  if (strlen(myData.name) != 0)
    WiFi.hostname(myData.name); //Set DNS hostname

  WiFi.begin(myData.ssid.c_str(), myData.psk.c_str());
  CONSOLELN(F("Rescued Wifi credentials"));

  CONSOLE(F("   -> waited for "));
  unsigned long startedAt = millis();
  // int connRes = WiFi.waitForConnectResult();
  uint8_t wait = 0;
  while (WiFi.status() == WL_DISCONNECTED)
  {
    delay(200);
    wait++;
    if (wait > 50)
      break;
  }
  auto waited = (millis() - startedAt);
  CONSOLE(waited);
  CONSOLE(F("ms, result "));
  CONSOLELN(WiFi.status());

  if (WiFi.status() == WL_DISCONNECTED)
    return false;
  else
    return true;
}

void setup()
{

  Serial.begin(115200);

  CONSOLELN(F("\nFW " FIRMWAREVERSION));
  CONSOLELN(ESP.getSdkVersion());
  
  // Diagnostyka stanu początkowego
  CONSOLE(F("Początkowa wolna pamięć: "));
  CONSOLELN(ESP.getFreeHeap());
  CONSOLE(F("Reset reason: "));
  CONSOLELN(ESP.getResetReason());
  CONSOLE(F("Flash size: "));
  CONSOLELN(ESP.getFlashChipSize());
  CONSOLE(F("CPU freq: "));
  CONSOLELN(ESP.getCpuFreqMHz());
  
  sleepManager();

  bool validConf = readConfig();
  if (!validConf)
    CONSOLELN(F("\nERROR config corrupted"));
  initDS18B20();
  initAccel();

#ifdef WIFI_IS_OFF_AT_BOOT
  // persistence is disabled by default and WiFi does not start automatically at boot
  // source: https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/generic-class.html#persistent
  enableWiFiAtBootTime();
#endif

  // decide whether we want configuration mode or normal mode
  if (shouldStartConfig(validConf))
  {
    uint32_t tmp;
    ESP.rtcUserMemoryRead(WIFIENADDR, &tmp, sizeof(tmp));

    // DIRTY hack to keep track of WAKE_RF_DEFAULT --> find a way to read WAKE_RF_*
    if (tmp != RTCVALIDFLAG)
    {
      drd.setRecentlyResetFlag();
      tmp = RTCVALIDFLAG;
      ESP.rtcUserMemoryWrite(WIFIENADDR, &tmp, sizeof(tmp));
      CONSOLELN(F("reboot RFCAL"));
      ESP.deepSleep(100000, WAKE_RFCAL);
      delay(500);
    }
    else
    {
      tmp = 0;
      ESP.rtcUserMemoryWrite(WIFIENADDR, &tmp, sizeof(tmp));
    }

    flasher.attach(1, flash);

    // rescue if wifi credentials lost because of power loss
    if (!startConfiguration())
    {
      // test if ssid exists
      if (WiFi.SSID() == "" && myData.ssid != "" && myData.psk != "")
      {
        connectBackupCredentials();
      }
    }
    uint32_t left2sleep = 0;
    ESP.rtcUserMemoryWrite(RTCSLEEPADDR, &left2sleep, sizeof(left2sleep));

    flasher.detach();
  }
  // to make sure we wake up with STA but AP
  WiFi.mode(WIFI_STA);
  Volt = getBattery();
  // we try to survive
  if (isSafeMode(Volt))
    WiFi.setOutputPower(0);
  else
    WiFi.setOutputPower(20.5);

  Tilt = getTilt();

  float accTemp = accelgyro.getTemperature() / 340.00 + 36.53;
  accelgyro.setSleepEnabled(true);

  CONSOLE("x: ");
  CONSOLE(ax);
  CONSOLE(" y: ");
  CONSOLE(ay);
  CONSOLE(" z: ");
  CONSOLELN(az);

  CONSOLE(F("Tilt: "));
  CONSOLELN(Tilt);
  CONSOLE("Tacc: ");
  CONSOLELN(accTemp);
  CONSOLE("Volt: ");
  CONSOLELN(Volt);

  // call as late as possible since DS needs converge time
  Temperatur = getTemperature(true);
  CONSOLE("Temp: ");
  CONSOLELN(Temperatur);

  // calc gravity on user defined polynominal
  Gravity = calculateGravity();
  CONSOLE(F("Gravity: "));
  CONSOLELN(Gravity);

  if (WiFi.status() != WL_CONNECTED)
  {
    unsigned long startedAt = millis();
    CONSOLE(F("After waiting "));
    uint8_t wait = 0;
    while (WiFi.status() == WL_DISCONNECTED)
    {
      delay(200);
      wait++;
      if (wait > 50)
        break;
    }
    auto waited = (millis() - startedAt);
    CONSOLE(waited);
    CONSOLE(F("ms, result "));
    CONSOLELN(WiFi.status());
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    CONSOLE(F("IP: "));
    CONSOLELN(WiFi.localIP());
    uploadData(myData.api);
  }
  else
  {
    CONSOLELN(F("Failed to connect -> trying to restore connection..."));

    if (connectBackupCredentials())
      CONSOLE(F("   -> Connection restored!"));
    else
      CONSOLE(F("   -> Failed to restore connection..."));
  }

  // survive - 60min sleep time
  if (isSafeMode(Volt))
  {
    myData.sleeptime = EMERGENCYSLEEP;
  }

  goodNight(myData.sleeptime);
}

void loop()
{
  CONSOLELN(F("should never be here!"));
}

const char* getStatus(int status)
{
    switch (status)
    {
    case WL_IDLE_STATUS:
        return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL:
        return "WL_NO_SSID_AVAIL";
    case WL_CONNECTED:
        return "WL_CONNECTED";
    case WL_CONNECT_FAILED:
        return "WL_CONNECT_FAILED";
    case WL_DISCONNECTED:
        return "WL_DISCONNECTED";
    default:
        return "UNKNOWN";
    }
}
