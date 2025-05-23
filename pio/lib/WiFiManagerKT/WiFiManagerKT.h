/**************************************************************
   WiFiManager is a library for the ESP8266/Arduino platform
   (https://github.com/esp8266/Arduino) to enable easy
   configuration and reconfiguration of WiFi credentials using a Captive Portal
   inspired by:
   http://www.esp8266.com/viewtopic.php?f=29&t=2520
   https://github.com/chriscook8/esp-arduino-apboot
   https://github.com/esp8266/Arduino/tree/esp8266/hardware/esp8266com/esp8266/libraries/DNSServer/examples/CaptivePortalAdvanced
   This version built by Ken Taylor https://github.com/kentaylor
   Licensed under MIT license
 **************************************************************/

#ifndef WiFiManager_h
#define WiFiManager_h

#include "Globals.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <I2Cdev.h>
#include "MPUOffset.h"
#include <memory>
#undef min
#undef max
#include <algorithm>
extern "C" {
#include "user_interface.h"
}

#define WFM_LABEL_BEFORE 1
#define WFM_LABEL_AFTER 2
#define WFM_NO_LABEL 0

const char HTTP_ISPINDEL_IMG[] PROGMEM = "<img src=\"data:image/jpeg;base64,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\" style=\"padding: 40px; display: block; width: 80%;\"/>";

const char HTTP_200[] PROGMEM = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

const char HTTP_STYLE[] PROGMEM = "<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;} .l{background: url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==\") no-repeat left center;background-size: 1em;}</style>";

const char HTTP_SCRIPT[] PROGMEM = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";

const char HTTP_HEAD_END[] PROGMEM = "</head><body><div style='text-align:left;display:inline-block;min-width:260px;'>";
const char HTTP_PORTAL_OPTIONS[] PROGMEM = "<form action=\"/wifi\" method=\"get\"><button>Konfiguracja WiFi</button></form><br/><form action=\"/0wifi\" method=\"get\"><button>Konfiguracja WiFi (bez skanowania)</button></form><br/><form action=\"/i\" method=\"get\"><button>Informacje</button></form><br/><form action=\"/r\" method=\"post\"><button>Reset</button></form>";
const char HTTP_ITEM[] PROGMEM = "<div><a href=\"#p\" onclick=\"c(this)\">{v}</a>&nbsp;<span class=\"q {i}\">{r}%</span></div>";
const char JSON_ITEM[] PROGMEM = "{\"SSID\":\"{v}\", \"Encryption\":{i}, \"Quality\":\"{r}\"}";
const char HTTP_FORM_START1[] PROGMEM = "<form method=\"get\" action=\"wifisave\"><label>Nazwa sieci WiFi</label><input id=\"s\" name=\"s\" length=32 placeholder=\"Nazwa sieci\" value=\"";
const char HTTP_FORM_START2[] PROGMEM = "\"><label>Hasło</label><input id=\"p\" name=\"p\" type=\"password\" length=64 placeholder=\"hasło\" value=\"";
const char HTTP_FORM_START3[] PROGMEM = "\">";

const char HTTP_FORM_LABEL[] PROGMEM = "<label for=\"{i}\">{p}</label>";
const char HTTP_FORM_PARAM[] PROGMEM = "<input id=\"{i}\" name=\"{n}\" length={l} placeholder=\"{p}\" value=\"{v}\" {c}>";
const char HTTP_FORM_END[] PROGMEM = "<button class=\"btn\" type=\"submit\">Zapisz</button></form>";
const char HTTP_SAVED[] PROGMEM = "<div class=\"msg\"><strong>Dane zapisane</strong><br>Próbuję połączyć ESP z siecią {x}.<br>Daj mi około 10 sekund i sprawdź <a href=\"/\">jak poszło.</a> <p/>Sieć {v}, z którą jesteś połączony zostanie zrestartowana na kanale radiowym sieci {x} i będzie kontynuować pracę w trybie normalnym, więc nie będzie dostępna, dopóki nie zostanie ręcznie zresetowana do trybu konfiguracji.";
const char HTTP_END[] PROGMEM = "</div></body></html>";
const char HTTP_UPDATE_FAI[] PROGMEM = "Aktualizacja nieudana!";
const char HTTP_UPDATE_SUC[] PROGMEM = "Aktualizacja zakończona sukcesem! Restartuję...";

const char HTTP_STATUS_ON[] PROGMEM = "<div class=\"msg\"><strong>Połączony</strong> z siecią {v}.<br><br><form action=\"/wifi\" method=\"get\"><button>Konfiguracja</button></form>";
const char HTTP_STATUS_OFF[] PROGMEM = "<div class=\"msg\"><strong>Brak połączenia</strong> z siecią {v}.<br><br><form action=\"/wifi\" method=\"get\"><button>Konfiguracja</button></form>";

#define WIFI_MANAGER_MAX_PARAMS 25

class WiFiManagerParameter
{
public:
  WiFiManagerParameter(const char *custom);
  WiFiManagerParameter(const char *id, const char *placeholder, const char *defaultValue, int length);
  WiFiManagerParameter(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom);
  WiFiManagerParameter(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom, int labelPlacement);

  const char *getID();
  const char *getValue();
  const char *getPlaceholder();
  int getValueLength();
  int getLabelPlacement();
  const char *getCustomHTML();

private:
  const char *_id;
  const char *_placeholder;
  char *_value;
  int _length;
  int _labelPlacement;
  const char *_customHTML;

  void init(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom, int labelPlacement);

  friend class WiFiManager;
};

class WiFiManager
{
public:
  WiFiManager();
  ~WiFiManager();

  boolean autoConnect();                                                  //Deprecated. Do not use.
  boolean autoConnect(char const *apName, char const *apPassword = NULL); //Deprecated. Do not use.

  //if you want to start the config portal
  boolean startConfigPortal();
  boolean startConfigPortal(char const *apName, char const *apPassword = NULL);

  // get the AP name of the config portal, so it can be used in the callback
  String getConfigPortalSSID();

  void header();

  void resetSettings();

  //sets timeout before webserver loop ends and exits even if there has been no setup.
  //usefully for devices that failed to connect at some point and got stuck in a webserver loop
  //in seconds setConfigPortalTimeout is a new name for setTimeout
  void setConfigPortalTimeout(unsigned long seconds);
  void setTimeout(unsigned long seconds);

  //sets timeout for which to attempt connecting, usefull if you get a lot of failed connects
  void setConnectTimeout(unsigned long seconds);

  void setDebugOutput(boolean debug);
  //defaults to not showing anything under 8% signal quality if called
  void setMinimumSignalQuality(int quality = 8);
  //sets a custom ip /gateway /subnet configuration
  void setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn);
  //sets config for a static IP
  void setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn);
  //called when AP mode and config portal is started
  void setAPCallback(void (*func)(WiFiManager *));
  //called when settings have been changed and connection was successful
  void setSaveConfigCallback(void (*func)(void));
  //adds a custom parameter
  void addParameter(WiFiManagerParameter *p);
  //if this is set, it will exit after config, even if connection is unsucessful.
  void setBreakAfterConfig(boolean shouldBreak);
  //if this is set, try WPS setup when starting (this will delay config portal for up to 2 mins)
  //TODO
  //if this is set, customise style
  void setCustomHeadElement(const char *element);
  //if this is true, remove duplicated Access Points - defaut true
  void setRemoveDuplicateAPs(boolean removeDuplicates);
  //Scan for WiFiNetworks in range and sort by signal strength
  //space for indices array allocated on the heap and should be freed when no longer required
  int scanWifiNetworks(int **indicesptr);

  void setConfSSID(String ssid) { _ssid = ssid; }
  void setConfPSK(String psk) { _pass = psk; }

private:
  String _ssid = "";
  String _pass = "";
  std::unique_ptr<DNSServer> dnsServer;
  std::unique_ptr<ESP8266WebServer> server;

  void setupConfigPortal();
  void startWPS();
  const char *getStatus(int status);

  const char *_apName = "no-net";
  const char *_apPassword = NULL;
  unsigned long _configPortalTimeout = 0;
  unsigned long _connectTimeout = 0;
  unsigned long _configPortalStart = 0;
  /* hostname for mDNS. Set to a valid internet address so that user
    will see an information page if they are connected to the wrong network */
  const char *myHostname = "192.168.4.1";
  int numberOfNetworks;
  int *networkIndices;
  int **networkIndicesptr = &networkIndices;

  IPAddress _ap_static_ip;
  IPAddress _ap_static_gw;
  IPAddress _ap_static_sn;
  IPAddress _sta_static_ip;
  IPAddress _sta_static_gw;
  IPAddress _sta_static_sn;

  int _paramsCount = 0;
  int _minimumQuality = -1;
  boolean _removeDuplicateAPs = true;
  boolean _shouldBreakAfterConfig = false;
  boolean _tryWPS = false;

  const char *_customHeadElement = "";

  //String        getEEPROMString(int start, int len);
  //void          setEEPROMString(int start, int len, String string);

  int status = WL_IDLE_STATUS;
  int connectWifi(String ssid, String pass);
  uint8_t waitForConnectResult();

  void handleRoot();
  void handleWifi();
  void handleWifiSave();
  void handleServerClose();
  void handleInfo();
  void handleiSpindel();
  void handleState();
  void handleScan();
  void handleReset();
  void handleNotFound();
  void handleMnt();
  void handleOffset();
  void handleUpdating();
  void handleUpdateDone();
  void handleCalibrateVref();
  boolean captivePortal();
  void reportStatus(String &page);

  // DNS server
  const byte DNS_PORT = 53;

  //helpers
  int getRSSIasQuality(int RSSI);
  boolean isIp(String str);
  String toStringIp(IPAddress ip);

  boolean connect;
  boolean stopConfigPortal = false;
  boolean _debug = true;

  void (*_apcallback)(WiFiManager *) = NULL;
  void (*_savecallback)(void) = NULL;

  WiFiManagerParameter *_params[WIFI_MANAGER_MAX_PARAMS];

  template <typename Generic>
  void DEBUG_WM(Generic text);

  template <class T>
  auto optionalIPFromString(T *obj, const char *s) -> decltype(obj->fromString(s))
  {
    return obj->fromString(s);
  }
  auto optionalIPFromString(...) -> bool
  {
    DEBUG_WM("NO fromString METHOD ON IPAddress, you need ESP8266 core 2.1.0 or newer for Custom IP configuration to work.");
    return false;
  }
};

#endif
