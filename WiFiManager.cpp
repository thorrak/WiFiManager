/**
 * WiFiManager.cpp
 * 
 * WiFiManager, a library for the ESP8266/Arduino platform
 * for configuration of WiFi credentials using a Captive Portal
 * 
 * @author Creator tzapu
 * @author tablatronix
 * @version 0.0.0
 * @license MIT
 */

#include "WiFiManager.h"
#include <string>

#if defined(ESP8266) || defined(ESP32)

#ifdef ESP32
uint8_t WiFiManager::_lastconxresulttmp = WL_IDLE_STATUS;
#endif

/**
 * --------------------------------------------------------------------------------
 *  WiFiManagerParameter
 * --------------------------------------------------------------------------------
**/

WiFiManagerParameter::WiFiManagerParameter() {
  WiFiManagerParameter("");
}

WiFiManagerParameter::WiFiManagerParameter(const char *custom) {
  _id             = NULL;
  _label          = NULL;
  _length         = 0;
  _value          = nullptr;
  _labelPlacement = WFM_LABEL_DEFAULT;
  _customHTML     = custom;
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label) {
  init(id, label, "", 0, "", WFM_LABEL_DEFAULT);
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length) {
  init(id, label, defaultValue, length, "", WFM_LABEL_DEFAULT);
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom) {
  init(id, label, defaultValue, length, custom, WFM_LABEL_DEFAULT);
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement) {
  init(id, label, defaultValue, length, custom, labelPlacement);
}

void WiFiManagerParameter::init(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement) {
  _id             = id;
  _label          = label;
  _labelPlacement = labelPlacement;
  _customHTML     = custom;
  _length         = 0;
  _value          = nullptr;
  setValue(defaultValue,length);
}

WiFiManagerParameter::~WiFiManagerParameter() {
  if (_value != NULL) {
    delete[] _value;
  }
  _length=0; // setting length 0, ideally the entire parameter should be removed, or added to wifimanager scope so it follows
}

// WiFiManagerParameter& WiFiManagerParameter::operator=(const WiFiManagerParameter& rhs){
//   Serial.println("copy assignment op called");
//   (*this->_value) = (*rhs._value);
//   return *this;
// }

// @note debug is not available in wmparameter class
void WiFiManagerParameter::setValue(const char *defaultValue, int length) {
  if(!_id){
    // Serial.println("cannot set value of this parameter");
    return;
  }
  
  // if(strlen(defaultValue) > length){
  //   // Serial.println("defaultValue length mismatch");
  //   // return false; //@todo bail 
  // }

  if(_length != length || _value == nullptr){
    _length = length;
    if( _value != nullptr){
      delete[] _value;
    }
    _value  = new char[_length + 1];  
  }

  memset(_value, 0, _length + 1); // explicit null
  
  if (defaultValue != NULL) {
    strncpy(_value, defaultValue, _length);
  }
}
const char* WiFiManagerParameter::getValue() const {
  // Serial.println(printf("Address of _value is %p\n", (void *)_value)); 
  return _value;
}
const char* WiFiManagerParameter::getID() const {
  return _id;
}
const char* WiFiManagerParameter::getPlaceholder() const {
  return _label;
}
const char* WiFiManagerParameter::getLabel() const {
  return _label;
}
int WiFiManagerParameter::getValueLength() const {
  return _length;
}
int WiFiManagerParameter::getLabelPlacement() const {
  return _labelPlacement;
}
const char* WiFiManagerParameter::getCustomHTML() const {
  return _customHTML;
}

/**
 * [addParameter description]
 * @access public
 * @param {[type]} WiFiManagerParameter *p [description]
 */
bool WiFiManager::addParameter(WiFiManagerParameter *p) {

  // check param id is valid, unless null
  if(p->getID()){
    for (size_t i = 0; i < strlen(p->getID()); i++){
       if(!(isAlphaNumeric(p->getID()[i])) && !(p->getID()[i]=='_')){
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] parameter IDs can only contain alpha numeric chars");
        #endif
        return false;
       }
    }
  }

  // init params if never malloc
  if(_params == NULL){
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_DEV,"allocating params bytes:",_max_params * sizeof(WiFiManagerParameter*));        
    #endif
    _params = (WiFiManagerParameter**)malloc(_max_params * sizeof(WiFiManagerParameter*));
  }

  // resize the params array by increment of WIFI_MANAGER_MAX_PARAMS
  if(_paramsCount == _max_params){
    _max_params += WIFI_MANAGER_MAX_PARAMS;
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_DEV,"Updated _max_params:",_max_params);
    DEBUG_WM(WM_DEBUG_DEV,"re-allocating params bytes:",_max_params * sizeof(WiFiManagerParameter*));    
    #endif
    WiFiManagerParameter** new_params = (WiFiManagerParameter**)realloc(_params, _max_params * sizeof(WiFiManagerParameter*));
    #ifdef WM_DEBUG_LEVEL
    // DEBUG_WM(WIFI_MANAGER_MAX_PARAMS);
    // DEBUG_WM(_paramsCount);
    // DEBUG_WM(_max_params);
    #endif
    if (new_params != NULL) {
      _params = new_params;
    } else {
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] failed to realloc params, size not increased!");
      #endif
      return false;
    }
  }

  _params[_paramsCount] = p;
  _paramsCount++;
  
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"Added Parameter:",p->getID());
  #endif
  return true;
}

/**
 * [getParameters description]
 * @access public
 */
WiFiManagerParameter** WiFiManager::getParameters() {
  return _params;
}

/**
 * [getParametersCount description]
 * @access public
 */
int WiFiManager::getParametersCount() {
  return _paramsCount;
}

/**
 * --------------------------------------------------------------------------------
 *  WiFiManager 
 * --------------------------------------------------------------------------------
**/

// constructors
WiFiManager::WiFiManager(Print& consolePort):_debugPort(consolePort){
  WiFiManagerInit();
}

WiFiManager::WiFiManager() {
  WiFiManagerInit();  
}

void WiFiManager::WiFiManagerInit(){
  setMenu(_menuIdsDefault);
  if(_debug && _debugLevel >= WM_DEBUG_DEV) debugPlatformInfo();
  _max_params = WIFI_MANAGER_MAX_PARAMS;
}

// destructor
WiFiManager::~WiFiManager() {
  _end();
  // parameters
  // @todo below belongs to wifimanagerparameter
  if (_params != NULL){
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_DEV,"freeing allocated params!");
    #endif
    free(_params);
    _params = NULL;
  }

  // remove event
  // WiFi.onEvent(std::bind(&WiFiManager::WiFiEvent,this,_1,_2));
  #ifdef ESP32
    WiFi.removeEvent(wm_event_id);
  #endif

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV,"unloading");
  #endif
}

void WiFiManager::_begin(){
  if(_hasBegun) return;
  _hasBegun = true;
  // _usermode = WiFi.getMode();

  #ifndef ESP32
  WiFi.persistent(false); // disable persistent so scannetworks and mode switching do not cause overwrites
  #endif
}

void WiFiManager::_end(){
  _hasBegun = false;
  if(_userpersistent) WiFi.persistent(true); // reenable persistent, there is no getter we rely on _userpersistent
  // if(_usermode != WIFI_OFF) WiFi.mode(_usermode);
}

// AUTOCONNECT

boolean WiFiManager::autoConnect() {
  std::string ssid = getDefaultAPName();
  return autoConnect(ssid.c_str(), NULL);
}

/**
 * [autoConnect description]
 * @access public
 * @param  {[type]} char const         *apName     [description]
 * @param  {[type]} char const         *apPassword [description]
 * @return {[type]}      [description]
 */
boolean WiFiManager::autoConnect(char const *apName, char const *apPassword) {
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM("AutoConnect");
  #endif

  // bool wifiIsSaved = getWiFiIsSaved();
  bool wifiIsSaved = true; // workaround until I can check esp32 wifiisinit and has nvs

  #ifdef ESP32
  setupHostname(true);

  if(_hostname != ""){
    // disable wifi if already on
    if(WiFi.getMode() & WIFI_STA){
      WiFi.mode(WIFI_OFF);
      int timeout = millis()+1200;
      // async loop for mode change
      while(WiFi.getMode()!= WIFI_OFF && millis()<timeout){
        delay(0);
      }
    }
  }
  #endif

  // check if wifi is saved, (has autoconnect) to speed up cp start
  // NOT wifi init safe
  if(wifiIsSaved){
     _startconn = millis();
    _begin();

    // attempt to connect using saved settings, on fail fallback to AP config portal
    if(!WiFi.enableSTA(true)){
      // handle failure mode Brownout detector etc.
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_ERROR,"[FATAL] Unable to enable wifi!");
      #endif
      return false;
    }
    
    WiFiSetCountry();

    #ifdef ESP32
    if(esp32persistent) WiFi.persistent(false); // disable persistent for esp32 after esp_wifi_start or else saves wont work
    #endif

    _usermode = WIFI_STA; // When using autoconnect , assume the user wants sta mode on permanently.

    // no getter for autoreconnectpolicy before this
    // https://github.com/esp8266/Arduino/pull/4359
    // so we must force it on else, if not connectimeout then waitforconnectionresult gets stuck endless loop
    WiFi_autoReconnect();

    #ifdef ESP8266
    if(_hostname != ""){
      setupHostname(true);
    }
    #endif

    // if already connected, or try stored connect 
    // @note @todo ESP32 has no autoconnect, so connectwifi will always be called unless user called begin etc before
    // @todo check if correct ssid == saved ssid when already connected
    bool connected = false;
    if (WiFi.status() == WL_CONNECTED){
      connected = true;
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM("AutoConnect: ESP Already Connected");
      #endif
      setSTAConfig();
      // @todo not sure if this is safe, causes dup setSTAConfig in connectwifi,
      // and we have no idea WHAT we are connected to
    }

    if(connected || connectWifi(_defaultssid, _defaultpass) == WL_CONNECTED){
      //connected
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM("AutoConnect: SUCCESS");
      DEBUG_WM(WM_DEBUG_VERBOSE,"Connected in",(std::string)((millis()-_startconn)) + " ms");
      DEBUG_WM("STA IP Address:",WiFi.localIP());
      #endif
      // Serial.println("Connected in " + (std::string)((millis()-_startconn)) + " ms");
      _lastconxresult = WL_CONNECTED;

      if(_hostname != ""){
        #ifdef WM_DEBUG_LEVEL
          DEBUG_WM(WM_DEBUG_DEV,"hostname: STA: ",getWiFiHostname());
        #endif
      }
      return true; // connected success
    }

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM("AutoConnect: FAILED for ",(std::string)((millis()-_startconn)) + " ms");
    #endif
  }
  else {
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM("No Credentials are Saved, skipping connect");
    #endif
  }

  // possibly skip the config portal
  if (!_enableConfigPortal) {
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"enableConfigPortal: FALSE, skipping ");
    #endif

    return false; // not connected and not cp
  }

  // not connected start configportal
  bool res = startConfigPortal(apName, apPassword);
  return res;
}

bool WiFiManager::setupHostname(bool restart){
  if(_hostname == "") {
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_DEV,"No Hostname to set");
    #endif
    return false;
  } 
  else {
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"Setting Hostnames: ",_hostname);
    #endif
  }
  bool res = true;
  #ifdef ESP8266
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"Setting WiFi hostname");
    #endif
    res = WiFi.hostname(_hostname.c_str());
    // #ifdef ESP8266MDNS_H
    #ifdef WM_MDNS
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"Setting MDNS hostname, tcp 80");
      #endif
      if(MDNS.begin(_hostname.c_str())){
        MDNS.addService("http", "tcp", 80);
      }
    #endif
  #elif defined(ESP32)
    // @note hostname must be set after STA_START
    // @note, this may have changed at some point, now it wont work, I have to set it before.
    // same for S2, must set it before mode(STA) now
  
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"Setting WiFi hostname");
    #endif

    res = WiFi.setHostname(_hostname.c_str());
    // esp_err_t err;
    //   // err = set_esp_interface_hostname(ESP_IF_WIFI_STA, "TEST_HOSTNAME");
    //   err = esp_netif_set_hostname(esp_netifs[ESP_IF_WIFI_STA], "TEST_HOSTNAME");
    //     if(err){
    //         log_e("Could not set hostname! %d", err);
    //         return false;
    //     } 
    // #ifdef ESP32MDNS_H
      #ifdef WM_MDNS
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_VERBOSE,"Setting MDNS hostname, tcp 80");
        #endif
      if(MDNS.begin(_hostname.c_str())){
        MDNS.addService("http", "tcp", 80);
      }
    #endif
  #endif

  #ifdef WM_DEBUG_LEVEL
  if(!res)DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] hostname: set failed!");
  #endif

  if(restart && (WiFi.status() == WL_CONNECTED)){
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"reconnecting to set new hostname");
    #endif
    // WiFi.reconnect(); // This does not reset dhcp
    WiFi_Disconnect();
    delay(200); // do not remove, need a delay for disconnect to change status()
  }

  return res;
}

// CONFIG PORTAL
bool WiFiManager::startAP(){
  bool ret = true;
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM("StartAP with SSID: ",_apName);
  #endif

  #ifdef ESP8266
    // @bug workaround for bug #4372 https://github.com/esp8266/Arduino/issues/4372
    if(!WiFi.enableAP(true)) {
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] enableAP failed!");
      #endif
      return false;
    }
    delay(500); // workaround delay
  #endif

  // setup optional soft AP static ip config
  if (_ap_static_ip) {
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM("Custom AP IP/GW/Subnet:");
    #endif
    if(!WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn)){
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] softAPConfig failed!");
      #endif
    }
  }

  //@todo add callback here if needed to modify ap but cannot use setAPStaticIPConfig
  //@todo rework wifi channelsync as it will work unpredictably when not connected in sta
 
  int32_t channel = 0;
  if(_channelSync) channel = WiFi.channel();
  else channel = _apChannel;

  if(channel>0){
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"Starting AP on channel:",channel);
    #endif
  }

  // start soft AP with password or anonymous
  // default channel is 1 here and in esplib, @todo just change to default remove conditionals
  if (_apPassword != "") {
    if(channel>0){
      ret = WiFi.softAP(_apName.c_str(), _apPassword.c_str(),channel,_apHidden);
    }  
    else{
      ret = WiFi.softAP(_apName.c_str(), _apPassword.c_str(),1,_apHidden);//password option
    }
  } else {
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"AP has anonymous access!");    
    #endif
    if(channel>0){
      ret = WiFi.softAP(_apName.c_str(),"",channel,_apHidden);
    }  
    else{
      ret = WiFi.softAP(_apName.c_str(),"",1,_apHidden);
    }  
  }

  if(_debugLevel >= WM_DEBUG_DEV) debugSoftAPConfig();

  // @todo add softAP retry here to dela with unknown failures
  
  delay(500); // slight delay to make sure we get an AP IP
  #ifdef WM_DEBUG_LEVEL
  if(!ret) DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] There was a problem starting the AP");
  DEBUG_WM("AP IP address:",WiFi.softAPIP());
  #endif

  // set ap hostname
  #ifdef ESP32
    if(ret && _hostname != ""){
      bool res =  WiFi.softAPsetHostname(_hostname.c_str());
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"setting softAP Hostname:",_hostname);
      if(!res)DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] hostname: AP set failed!");
      DEBUG_WM(WM_DEBUG_DEV,"hostname: AP: ",WiFi.softAPgetHostname());
      #endif
   }
  #endif

  return ret;
}

/**
 * [startWebPortal description]
 * @access public
 * @return {[type]} [description]
 */
void WiFiManager::startWebPortal() {
  if(configPortalActive || webPortalActive) return;
  connect = abort = false;
  setupConfigPortal();
  webPortalActive = true;
}

/**
 * [stopWebPortal description]
 * @access public
 * @return {[type]} [description]
 */
void WiFiManager::stopWebPortal() {
  if(!configPortalActive && !webPortalActive) return;
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"Stopping Web Portal");  
  #endif
  webPortalActive = false;
  shutdownConfigPortal();
}

boolean WiFiManager::configPortalHasTimeout(){
    if(!configPortalActive) return false;
    uint16_t logintvl = 30000; // how often to emit timeing out counter logging

    // handle timeout portal client check
    if(_configPortalTimeout == 0 || (_apClientCheck && (WiFi_softap_num_stations() > 0))){
      // debug num clients every 30s
      if(millis() - timer > logintvl){
        timer = millis();
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_VERBOSE,"NUM CLIENTS: ",(std::string)WiFi_softap_num_stations());
        #endif
      }
      _configPortalStart = millis(); // kludge, bump configportal start time to skew timeouts
      return false;
    }

    // handle timeout webclient check
    if(_webClientCheck && (_webPortalAccessed>_configPortalStart)>0) _configPortalStart = _webPortalAccessed;

    // handle timed out
    if(millis() > _configPortalStart + _configPortalTimeout){
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM("config portal has timed out");
      #endif
      return true; // timeout bail, else do debug logging
    } 
    else if(_debug && _debugLevel > 0) {
      // log timeout time remaining every 30s
      if((millis() - timer) > logintvl){
        timer = millis();
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_VERBOSE,"Portal Timeout In",(std::string)((_configPortalStart + _configPortalTimeout-millis())/1000) + (std::string)" seconds");
        #endif
      }
    }

    return false;
}

void WiFiManager::setupHTTPServer(){

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM("Starting Web Portal");
  #endif

  if(_httpPort != 80) {
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"http server started with custom port: ",_httpPort); // @todo not showing ip
    #endif
  }

  server.reset(new WM_WebServer(_httpPort));
  // This is not the safest way to reset the webserver, it can cause crashes on callbacks initilized before this and since its a shared pointer...

  if ( _webservercallback != NULL) {
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"[CB] _webservercallback calling");
    #endif
    _webservercallback(); // @CALLBACK
  }
  // @todo add a new callback maybe, after webserver started, callback cannot override handlers, but can grab them first
  
  /* Setup httpd callbacks, web pages: root, wifi config pages, SO captive portal detectors and not found. */

  // G macro workaround for Uri() bug https://github.com/esp8266/Arduino/issues/7102
  server->on(WM_G(R_root),       std::bind(&WiFiManager::handleRoot, this));
  server->on(WM_G(R_wifi),       std::bind(&WiFiManager::handleWifi, this, true));
  server->on(WM_G(R_wifinoscan), std::bind(&WiFiManager::handleWifi, this, false));
  server->on(WM_G(R_wifisave),   std::bind(&WiFiManager::handleWifiSave, this));
  server->on(WM_G(R_info),       std::bind(&WiFiManager::handleInfo, this));
  server->on(WM_G(R_param),      std::bind(&WiFiManager::handleParam, this));
  server->on(WM_G(R_paramsave),  std::bind(&WiFiManager::handleParamSave, this));
  server->on(WM_G(R_restart),    std::bind(&WiFiManager::handleReset, this));
  server->on(WM_G(R_exit),       std::bind(&WiFiManager::handleExit, this));
  server->on(WM_G(R_close),      std::bind(&WiFiManager::handleClose, this));
  server->on(WM_G(R_erase),      std::bind(&WiFiManager::handleErase, this, false));
  server->on(WM_G(R_status),     std::bind(&WiFiManager::handleWiFiStatus, this));
  server->onNotFound (std::bind(&WiFiManager::handleNotFound, this));
  
  server->on(WM_G(R_update), std::bind(&WiFiManager::handleUpdate, this));
  server->on(WM_G(R_updatedone), HTTP_POST, std::bind(&WiFiManager::handleUpdateDone, this), std::bind(&WiFiManager::handleUpdating, this));
  
  server->begin(); // Web server start
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"HTTP server started");
  #endif
}

void WiFiManager::setupDNSD(){
  dnsServer.reset(new DNSServer());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  #ifdef WM_DEBUG_LEVEL
  // DEBUG_WM("dns server started port: ",DNS_PORT);
  DEBUG_WM(WM_DEBUG_DEV,"dns server started with ip: ",WiFi.softAPIP()); // @todo not showing ip
  #endif
  dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
}

void WiFiManager::setupConfigPortal() {
  setupHTTPServer();
  _lastscan = 0; // reset network scan cache
  if(_preloadwifiscan) WiFi_scanNetworks(true,true); // preload wifiscan , async
}

boolean WiFiManager::startConfigPortal() {
  std::string ssid = getDefaultAPName();
  return startConfigPortal(ssid.c_str(), NULL);
}

/**
 * [startConfigPortal description]
 * @access public
 * @param  {[type]} char const         *apName     [description]
 * @param  {[type]} char const         *apPassword [description]
 * @return {[type]}      [description]
 */
boolean  WiFiManager::startConfigPortal(char const *apName, char const *apPassword) {
  _begin();

  if(configPortalActive){
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"Starting Config Portal FAILED, is already running");
    #endif    
    return false;
  }

  //setup AP
  _apName     = apName; // @todo check valid apname ?
  _apPassword = apPassword;
  
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"Starting Config Portal");
  #endif

  if(_apName == "") _apName = getDefaultAPName();

  if(!validApPassword()) return false;
  
  // HANDLE issues with STA connections, shutdown sta if not connected, or else this will hang channel scanning and softap will not respond
  if(_disableSTA || (!WiFi.isConnected() && _disableSTAConn)){
    // this fixes most ap problems, however, simply doing mode(WIFI_AP) does not work if sta connection is hanging, must `wifi_station_disconnect` 
    #ifdef WM_DISCONWORKAROUND
      WiFi.mode(WIFI_AP_STA);
    #endif
    WiFi_Disconnect();
    WiFi_enableSTA(false);
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"Disabling STA");
    #endif
  }
  else {
    // WiFi_enableSTA(true);
  }

  // init configportal globals to known states
  configPortalActive = true;
  bool result = connect = abort = false; // loop flags, connect true success, abort true break
  uint8_t state;

  _configPortalStart = millis();

  // start access point
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"Enabling AP");
  #endif
  startAP();
  WiFiSetCountry();

  // do AP callback if set
  if ( _apcallback != NULL) {
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"[CB] _apcallback calling");
    #endif
    _apcallback(this);
  }

  // init configportal
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV,"setupConfigPortal");
  #endif
  setupConfigPortal();

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV,"setupDNSD");
  #endif  
  setupDNSD();
  

  if(!_configPortalIsBlocking){
    #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"Config Portal Running, non blocking (processing)");
      if(_configPortalTimeout > 0) DEBUG_WM(WM_DEBUG_VERBOSE,"Portal Timeout In",(std::string)(_configPortalTimeout/1000) + (std::string)" seconds");
    #endif
    return result; // skip blocking loop
  }

  // enter blocking loop, waiting for config
  
  #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"Config Portal Running, blocking, waiting for clients...");
    if(_configPortalTimeout > 0) DEBUG_WM(WM_DEBUG_VERBOSE,"Portal Timeout In",(std::string)(_configPortalTimeout/1000) + (std::string)" seconds");
  #endif

  while(1){

    // if timed out or abort, break
    if(configPortalHasTimeout() || abort){
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_DEV,"configportal loop abort");
      #endif
      shutdownConfigPortal();
      result = abort ? portalAbortResult : portalTimeoutResult; // false, false
      if (_configportaltimeoutcallback != NULL) {
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_VERBOSE,"[CB] config portal timeout callback");
        #endif
        _configportaltimeoutcallback();  // @CALLBACK
      }
      break;
    }

    state = processConfigPortal();
    
    // status change, break
    // @todo what is this for, should be moved inside the processor
    // I think.. this is to detect autoconnect by esp in background, there are also many open issues about autoreconnect not working
    if(state != WL_IDLE_STATUS){
        result = (state == WL_CONNECTED); // true if connected
        DEBUG_WM(WM_DEBUG_DEV,"configportal loop break");
        break;
    }

    if(!configPortalActive) break;

    yield(); // watchdog
  }

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_NOTIFY,"config portal exiting");
  #endif
  return result;
}

/**
 * [process description]
 * @access public
 * @return bool connected
 */
boolean WiFiManager::process(){
    // process mdns, esp32 not required
    #if defined(WM_MDNS) && defined(ESP8266)
    MDNS.update();
    #endif
	
    if(webPortalActive || (configPortalActive && !_configPortalIsBlocking)){
      // if timed out or abort, break
      if(_allowExit && (configPortalHasTimeout() || abort)){
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_DEV,"process loop abort");
        #endif
        webPortalActive = false;
        shutdownConfigPortal();
        if (_configportaltimeoutcallback != NULL) {
          #ifdef WM_DEBUG_LEVEL
          DEBUG_WM(WM_DEBUG_VERBOSE,"[CB] config portal timeout callback");
          #endif
          _configportaltimeoutcallback();  // @CALLBACK
        }
        return false;
      }

      uint8_t state = processConfigPortal(); // state is WL_IDLE or WL_CONNECTED/FAILED
      return state == WL_CONNECTED;
    }
    return false;
}

/**
 * [processConfigPortal description]
 * using esp wl_status enums as returns for now, should be fine
 * returns WL_IDLE_STATUS or WL_CONNECTED/WL_CONNECT_FAILED upon connect/save flag
 * 
 * @return {[type]} [description]
 */
uint8_t WiFiManager::processConfigPortal(){
    if(configPortalActive){
      //DNS handler
      dnsServer->processNextRequest();
    }

    //HTTP handler
    server->handleClient();

    // Waiting for save...
    if(connect) {
      connect = false;
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"processing save");
      #endif
      if(_enableCaptivePortal) delay(_cpclosedelay); // keeps the captiveportal from closing to fast.

      // skip wifi if no ssid
      if(_ssid == ""){
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_VERBOSE,"No ssid, skipping wifi save");
        #endif
      }
      else{
        // attempt sta connection to submitted _ssid, _pass
        uint8_t res = connectWifi(_ssid, _pass, _connectonsave) == WL_CONNECTED;
        if (res || (!_connectonsave)) {
          #ifdef WM_DEBUG_LEVEL
          if(!_connectonsave){
            DEBUG_WM("SAVED with no connect to new AP");
          } else {
            DEBUG_WM("Connect to new AP [SUCCESS]");
            DEBUG_WM("Got IP Address:");
            DEBUG_WM(WiFi.localIP());
          }
          #endif

          if ( _savewificallback != NULL) {
            #ifdef WM_DEBUG_LEVEL
            DEBUG_WM(WM_DEBUG_VERBOSE,"[CB] _savewificallback calling");
            #endif
            _savewificallback(); // @CALLBACK
          }
          if(!_connectonsave) return WL_IDLE_STATUS;
          if(_disableConfigPortal) shutdownConfigPortal();
          return WL_CONNECTED; // CONNECT SUCCESS
        }
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] Connect to new AP Failed");
        #endif
      }
 
      if (_shouldBreakAfterConfig) {

        // do save callback
        // @todo this is more of an exiting callback than a save, clarify when this should actually occur
        // confirm or verify data was saved to make this more accurate callback
        if ( _savewificallback != NULL) {
          #ifdef WM_DEBUG_LEVEL
          DEBUG_WM(WM_DEBUG_VERBOSE,"[CB] WiFi/Param save callback");
          #endif
          _savewificallback(); // @CALLBACK
        }
        if(_disableConfigPortal) shutdownConfigPortal();
        return WL_CONNECT_FAILED; // CONNECT FAIL
      }
      else if(_configPortalIsBlocking){
        // clear save strings
        _ssid = "";
        _pass = "";
        // if connect fails, turn sta off to stabilize AP
        WiFi_Disconnect();
        WiFi_enableSTA(false);
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_VERBOSE,"Processing - Disabling STA");
        #endif
      }
      else{
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_VERBOSE,"Portal is non blocking - remaining open");
        #endif        
      }
    }

    return WL_IDLE_STATUS;
}

/**
 * [shutdownConfigPortal description]
 * @access public
 * @return bool success (softapdisconnect)
 */
bool WiFiManager::shutdownConfigPortal(){
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"shutdownConfigPortal");
  #endif

  if(webPortalActive) return false;

  if(configPortalActive){
    //DNS handler
    dnsServer->processNextRequest();
  }

  //HTTP handler
  server->handleClient();

  // @todo what is the proper way to shutdown and free the server up
  // debug - many open issues aobut port not clearing for use with other servers
  server->stop();
  server.reset();

  WiFi.scanDelete(); // free wifi scan results

  if(!configPortalActive) return false;

  dnsServer->stop(); //  free heap ?
  dnsServer.reset();

  // turn off AP
  // @todo bug workaround
  // https://github.com/esp8266/Arduino/issues/3793
  // [APdisconnect] set_config failed! *WM: disconnect configportal - softAPdisconnect failed
  // still no way to reproduce reliably

  bool ret = false;
  ret = WiFi.softAPdisconnect(false);
  
  #ifdef WM_DEBUG_LEVEL
  if(!ret)DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] disconnect configportal - softAPdisconnect FAILED");
  DEBUG_WM(WM_DEBUG_VERBOSE,"restoring usermode",getModeString(_usermode));
  #endif
  delay(1000);
  WiFi_Mode(_usermode); // restore users wifi mode, BUG https://github.com/esp8266/Arduino/issues/4372
  if(WiFi.status()==WL_IDLE_STATUS){
    WiFi.reconnect(); // restart wifi since we disconnected it in startconfigportal
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"WiFi Reconnect, was idle");
    #endif
  }
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"wifi status:",getWLStatusString(WiFi.status()));
  DEBUG_WM(WM_DEBUG_VERBOSE,"wifi mode:",getModeString(WiFi.getMode()));
  #endif
  configPortalActive = false;
  DEBUG_WM(WM_DEBUG_VERBOSE,"configportal closed");
  _end();
  return ret;
}

// @todo refactor this up into seperate functions
// one for connecting to flash , one for new client
// clean up, flow is convoluted, and causes bugs
uint8_t WiFiManager::connectWifi(std::string ssid, std::string pass, bool connect) {
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"Connecting as wifi client...");
  #endif
  uint8_t retry = 1;
  uint8_t connRes = (uint8_t)WL_NO_SSID_AVAIL;

  setSTAConfig();
  //@todo catch failures in set_config
  
  // make sure sta is on before `begin` so it does not call enablesta->mode while persistent is ON ( which would save WM AP state to eeprom !)
  // WiFi.setAutoReconnect(false);
  if(_cleanConnect) WiFi_Disconnect(); // disconnect before begin, in case anything is hung, this causes a 2 seconds delay for connect
  // @todo find out what status is when this is needed, can we detect it and handle it, say in between states or idle_status to avoid these

  // if retry without delay (via begin()), the IDF is still busy even after returning status
  // E (5130) wifi:sta is connecting, return error
  // [E][WiFiSTA.cpp:221] begin(): connect failed!

  while(retry <= _connectRetries && (connRes!=WL_CONNECTED)){
  if(_connectRetries > 1){
    if(_aggresiveReconn) delay(1000); // add idle time before recon
    #ifdef WM_DEBUG_LEVEL
      DEBUG_WM("Connect Wifi, ATTEMPT #",(std::string)retry+" of "+(std::string)_connectRetries); 
      #endif
  }
  // if ssid argument provided connect to that
  // NOTE: this also catches preload() _defaultssid @todo rework
  if (ssid != "") {
    wifiConnectNew(ssid,pass,connect);
    // @todo connect=false seems to disconnect sta in begin() so not sure if _connectonsave is useful at all
    // skip wait if not connecting
    // if(connect){
      if(_saveTimeout > 0){
        connRes = waitForConnectResult(_saveTimeout); // use default save timeout for saves to prevent bugs in esp->waitforconnectresult loop
      }
      else {
         connRes = waitForConnectResult();
      }
    // }
  }
  else {
    // connect using saved ssid if there is one
    if (WiFi_hasAutoConnect()) {
      wifiConnectDefault();
      connRes = waitForConnectResult();
    }
    else {
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM("No wifi saved, skipping");
      #endif
    }
  }

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"Connection result:",getWLStatusString(connRes));
  #endif
  retry++;
}

// WPS enabled? https://github.com/esp8266/Arduino/pull/4889
#ifdef NO_EXTRA_4K_HEAP
  // do WPS, if WPS options enabled and not connected and no password was supplied
  // @todo this seems like wrong place for this, is it a fallback or option?
  if (_tryWPS && connRes != WL_CONNECTED && pass == "") {
    startWPS();
    // should be connected at the end of WPS
    connRes = waitForConnectResult();
  }
#endif

  if(connRes != WL_SCAN_COMPLETED){
    updateConxResult(connRes);
  }

  return connRes;
}

/**
 * connect to a new wifi ap
 * @since $dev
 * @param  std::string ssid 
 * @param  std::string pass 
 * @return bool success
 * @return connect only save if false
 */
bool WiFiManager::wifiConnectNew(std::string ssid, std::string pass,bool connect){
  bool ret = false;
  #ifdef WM_DEBUG_LEVEL
  // DEBUG_WM(WM_DEBUG_DEV,"CONNECTED: ",WiFi.status() == WL_CONNECTED ? "Y" : "NO");
  DEBUG_WM("Connecting to NEW AP:",ssid);
  DEBUG_WM(WM_DEBUG_DEV,"Using Password:",pass);
  #endif
  WiFi_enableSTA(true,storeSTAmode); // storeSTAmode will also toggle STA on in default opmode (persistent) if true (default)
  WiFi.persistent(true);
  ret = WiFi.begin(ssid.c_str(), pass.c_str(), 0, NULL, connect);
  WiFi.persistent(false);
  #ifdef WM_DEBUG_LEVEL
  if(!ret) DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] wifi begin failed");
  #endif
  return ret;
}

/**
 * connect to stored wifi
 * @since dev
 * @return bool success
 */
bool WiFiManager::wifiConnectDefault(){
  bool ret = false;

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM("Connecting to SAVED AP:",WiFi_SSID(true));
  DEBUG_WM(WM_DEBUG_DEV,"Using Password:",WiFi_psk(true));
  #endif

  ret = WiFi_enableSTA(true,storeSTAmode);
  delay(500); // THIS DELAY ?

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV,"Mode after delay: ",getModeString(WiFi.getMode()));
  if(!ret) DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] wifi enableSta failed");
  #endif

  ret = WiFi.begin();

  #ifdef WM_DEBUG_LEVEL
  if(!ret) DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] wifi begin failed");
  #endif

  return ret;
}


/**
 * set sta config if set
 * @since $dev
 * @return bool success
 */
bool WiFiManager::setSTAConfig(){
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV,"STA static IP:",_sta_static_ip);  
  #endif
  bool ret = true;
  if (_sta_static_ip) {
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"Custom static IP/GW/Subnet/DNS");
      #endif
    if(_sta_static_dns) {
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"Custom static DNS");
      #endif
      ret = WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn, _sta_static_dns);
    }
    else {
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"Custom STA IP/GW/Subnet");
      #endif
      ret = WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn);
    }

      #ifdef WM_DEBUG_LEVEL
      if(!ret) DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] wifi config failed");
      else DEBUG_WM("STA IP set:",WiFi.localIP());
      #endif
  } 
  else {
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"setSTAConfig static ip not set, skipping");
      #endif
  }
  return ret;
}

// @todo change to getLastFailureReason and do not touch conxresult
void WiFiManager::updateConxResult(uint8_t status){
  // hack in wrong password detection
  _lastconxresult = status;
    #ifdef ESP8266
      if(_lastconxresult == WL_CONNECT_FAILED){
        if(wifi_station_get_connect_status() == STATION_WRONG_PASSWORD){
          _lastconxresult = WL_STATION_WRONG_PASSWORD;
        }
      }
    #elif defined(ESP32)
      // if(_lastconxresult == WL_CONNECT_FAILED){
      if(_lastconxresult == WL_CONNECT_FAILED || _lastconxresult == WL_DISCONNECTED){
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_DEV,"lastconxresulttmp:",getWLStatusString(_lastconxresulttmp));            
        #endif
        if(_lastconxresulttmp != WL_IDLE_STATUS){
          _lastconxresult    = _lastconxresulttmp;
          // _lastconxresulttmp = WL_IDLE_STATUS;
        }
      }
    DEBUG_WM(WM_DEBUG_DEV,"lastconxresult:",getWLStatusString(_lastconxresult));
    #endif
}

 
uint8_t WiFiManager::waitForConnectResult() {
  #ifdef WM_DEBUG_LEVEL
  if(_connectTimeout > 0) DEBUG_WM(WM_DEBUG_DEV,_connectTimeout,"ms connectTimeout set"); 
  #endif
  return waitForConnectResult(_connectTimeout);
}

/**
 * waitForConnectResult
 * @param  uint16_t timeout  in seconds
 * @return uint8_t  WL Status
 */
uint8_t WiFiManager::waitForConnectResult(uint32_t timeout) {
  if (timeout == 0){
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM("connectTimeout not set, ESP waitForConnectResult...");
    #endif
    return WiFi.waitForConnectResult();
  }

  unsigned long timeoutmillis = millis() + timeout;
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,timeout,"ms timeout, waiting for connect...");
  #endif
  uint8_t status = WiFi.status();
  
  while(millis() < timeoutmillis) {
    status = WiFi.status();
    // @todo detect additional states, connect happens, then dhcp then get ip, there is some delay here, make sure not to timeout if waiting on IP
    if (status == WL_CONNECTED || status == WL_CONNECT_FAILED) {
      return status;
    }
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM (WM_DEBUG_VERBOSE,".");
    #endif
    delay(100);
  }
  return status;
}

// WPS enabled? https://github.com/esp8266/Arduino/pull/4889
#ifdef NO_EXTRA_4K_HEAP
void WiFiManager::startWPS() {
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM("START WPS");
  #endif
  #ifdef ESP8266  
    WiFi.beginWPSConfig();
  #else
    // @todo
  #endif
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM("END WPS");
  #endif
}
#endif

std::string WiFiManager::getHTTPHead(std::string title, std::string classes){
  std::string page;
  page += HTTP_HEAD_START;
  page.replace(T_v, title);
  page += HTTP_SCRIPT;
  page += HTTP_STYLE;
  page += _customHeadElement;

  std::string p = HTTP_HEAD_END;
  if (_bodyClass != "") {
    if (classes != "") {
      classes += " ";  // add spacing, if necessary
    }
    classes += _bodyClass;  // add class str
  }
  p.replace(T_c, classes);
  page += p;

  if (_customBodyHeader) {
    page += _customBodyHeader;
  }

  return page;
}

std::string WiFiManager::getHTTPEnd() {
  std::string end = HTTP_END;

  if (_customBodyFooter) {
    end = std::string(_customBodyFooter) + end;
  }

  return end;
}

void WiFiManager::HTTPSend(const std::string &content){
  server->send(200, HTTP_HEAD_CT, content);
}

/** 
 * HTTPD handler for page requests
 */
void WiFiManager::handleRequest() {
  _webPortalAccessed = millis();

  // TESTING HTTPD AUTH RFC 2617
  // BASIC_AUTH will hold onto creds, hard to "logout", but convienent
  // DIGEST_AUTH will require new auth often, and nonce is random
  // bool authenticate(const char * username, const char * password);
  // bool authenticateDigest(const std::string& username, const std::string& H1);
  // void requestAuthentication(HTTPAuthMethod mode = BASIC_AUTH, const char* realm = NULL, const std::string& authFailMsg = std::string("") );

  // 2.3 NO AUTH available
  bool testauth = false;
  if(!testauth) return;
  
  DEBUG_WM(WM_DEBUG_DEV,"DOING AUTH");
  bool res = server->authenticate("admin","12345");
  if(!res){
    #ifndef WM_NOAUTH
    server->requestAuthentication(HTTPAuthMethod::BASIC_AUTH); // DIGEST_AUTH
    #endif
    DEBUG_WM(WM_DEBUG_DEV,"AUTH FAIL");
  }
}

/** 
 * HTTPD CALLBACK root or redirect to captive portal
 */
void WiFiManager::handleRoot() {
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"<- HTTP Root");
  #endif
  if (captivePortal()) return; // If captive portal redirect instead of displaying the page
  handleRequest();
  std::string page = getHTTPHead(_title, C_root); // @token options @todo replace options with title
  std::string str  = HTTP_ROOT_MAIN; // @todo custom title
  str.replace(T_t,_title);
  str.replace(T_v,configPortalActive ? _apName : (getWiFiHostname() + " - " + WiFi.localIP().toString())); // use ip if ap is not active for heading @todo use hostname?
  page += str;
  page += HTTP_PORTAL_OPTIONS;
  page += getMenuOut();
  reportStatus(page);
  page += getHTTPEnd();

  HTTPSend(page);
  if(_preloadwifiscan) WiFi_scanNetworks(_scancachetime,true); // preload wifiscan throttled, async
  // @todo buggy, captive portals make a query on every page load, causing this to run every time in addition to the real page load
  // I dont understand why, when you are already in the captive portal, I guess they want to know that its still up and not done or gone
  // if we can detect these and ignore them that would be great, since they come from the captive portal redirect maybe there is a refferer
}

/**
 * HTTPD CALLBACK Wifi config page handler
 */
void WiFiManager::handleWifi(boolean scan) {
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"<- HTTP Wifi");
  #endif
  handleRequest();
  std::string page = getHTTPHead(S_titlewifi, C_wifi); // @token titlewifi
  if (scan) {
    #ifdef WM_DEBUG_LEVEL
    // DEBUG_WM(WM_DEBUG_DEV,"refresh flag:",server->hasArg(F("refresh")));
    #endif
    WiFi_scanNetworks(server->hasArg("refresh"),false); //wifiscan, force if arg refresh
    page += getScanItemOut();
  }
  std::string pitem = "";

  pitem = HTTP_FORM_START;
  pitem.replace(T_v, "wifisave"); // set form action
  page += pitem;

  pitem = HTTP_FORM_WIFI;
  pitem.replace(T_v, WiFi_SSID());

  if(_showPassword){
    pitem.replace(T_p, WiFi_psk());
  }
  else if(WiFi_psk() != ""){
    pitem.replace(T_p,S_passph);    
  }
  else {
    pitem.replace(T_p,"");    
  }

  page += pitem;

  page += getStaticOut();
  page += HTTP_FORM_WIFI_END;
  if(_paramsInWifi && _paramsCount>0){
    page += HTTP_FORM_PARAM_HEAD;
    page += getParamOut();
  }
  page += HTTP_FORM_END;
  page += HTTP_SCAN_LINK;
  if(_showBack) page += HTTP_BACKBTN;
  reportStatus(page);
  page += getHTTPEnd();

  HTTPSend(page);

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV,"Sent config page");
  #endif
}

/**
 * HTTPD CALLBACK Wifi param page handler
 */
void WiFiManager::handleParam(){
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"<- HTTP Param");
  #endif
  handleRequest();
  std::string page = getHTTPHead(S_titleparam, C_param); // @token titlewifi

  std::string pitem = "";

  pitem = HTTP_FORM_START;
  pitem.replace(T_v, "paramsave");
  page += pitem;

  page += getParamOut();
  page += HTTP_FORM_END;
  if(_showBack) page += HTTP_BACKBTN;
  reportStatus(page);
  page += getHTTPEnd();

  HTTPSend(page);

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV,"Sent param page");
  #endif
}


std::string WiFiManager::getMenuOut(){
  std::string page;  

  for(auto menuId :_menuIds ){
    if((std::string)_menutokens[menuId] == "param" && _paramsCount == 0) continue; // no params set, omit params from menu, @todo this may be undesired by someone, use only menu to force?
    if((std::string)_menutokens[menuId] == "custom" && _customMenuHTML!=NULL){
      page += _customMenuHTML;
      continue;
    }
    page += HTTP_PORTAL_MENU[menuId];
    delay(0);
  }

  return page;
}

// // is it possible in softap mode to detect aps without scanning
// bool WiFiManager::WiFi_scanNetworksForAP(bool force){
//   WiFi_scanNetworks(force);
// }

void WiFiManager::WiFi_scanComplete(int networksFound){
  _lastscan = millis();
  _numNetworks = networksFound;
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"WiFi Scan ASYNC completed", "in "+(std::string)(_lastscan - _startscan)+" ms");  
  DEBUG_WM(WM_DEBUG_VERBOSE,"WiFi Scan ASYNC found:",_numNetworks);
  #endif
}

bool WiFiManager::WiFi_scanNetworks(){
  return WiFi_scanNetworks(false,false);
}
 
bool WiFiManager::WiFi_scanNetworks(unsigned int cachetime,bool async){
    return WiFi_scanNetworks(millis()-_lastscan > cachetime,async);
}
bool WiFiManager::WiFi_scanNetworks(unsigned int cachetime){
    return WiFi_scanNetworks(millis()-_lastscan > cachetime,false);
}
bool WiFiManager::WiFi_scanNetworks(bool force,bool async){
    #ifdef WM_DEBUG_LEVEL
    // DEBUG_WM(WM_DEBUG_DEV,"scanNetworks async:",async == true);
    // DEBUG_WM(WM_DEBUG_DEV,_numNetworks,(millis()-_lastscan ));
    // DEBUG_WM(WM_DEBUG_DEV,"scanNetworks force:",force == true);
    #endif

    // if 0 networks, rescan @note this was a kludge, now disabling to test real cause ( maybe wifi not init etc)
    // enable only if preload failed? 
    if(_numNetworks == 0 && _autoforcerescan){
      DEBUG_WM(WM_DEBUG_DEV,"NO APs found forcing new scan");
      force = true;
    }

    // if scan is empty or stale (last scantime > _scancachetime), this avoids fast reloading wifi page and constant scan delayed page loads appearing to freeze.
    if(!_lastscan || (_lastscan>0 && (millis()-_lastscan > _scancachetime))){
      force = true;
    }

    if(force){
      int8_t res;
      _startscan = millis();
      if(async && _asyncScan){
        #ifdef ESP8266
          #ifndef WM_NOASYNC // no async available < 2.4.0
          #ifdef WM_DEBUG_LEVEL
          DEBUG_WM(WM_DEBUG_VERBOSE,"WiFi Scan ASYNC started");
          #endif
          using namespace std::placeholders; // for `_1`
          WiFi.scanNetworksAsync(std::bind(&WiFiManager::WiFi_scanComplete,this,_1));
          #else
          DEBUG_WM(WM_DEBUG_VERBOSE,"WiFi Scan SYNC started");
          res = WiFi.scanNetworks();
          #endif
        #else
        #ifdef WM_DEBUG_LEVEL
          DEBUG_WM(WM_DEBUG_VERBOSE,"WiFi Scan ASYNC started");
          #endif
          res = WiFi.scanNetworks(true);
        #endif
        return false;
      }
      else{
        DEBUG_WM(WM_DEBUG_VERBOSE,"WiFi Scan SYNC started");
        res = WiFi.scanNetworks();
      }
      if(res == WIFI_SCAN_FAILED){
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] scan failed");
        #endif
      }  
      else if(res == WIFI_SCAN_RUNNING){
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] scan waiting");
        #endif
        while(WiFi.scanComplete() == WIFI_SCAN_RUNNING){
          #ifdef WM_DEBUG_LEVEL
          DEBUG_WM(WM_DEBUG_ERROR,".");
          #endif
          delay(100);
        }
        _numNetworks = WiFi.scanComplete();
      }
      else if(res >=0 ) _numNetworks = res;
      _lastscan = millis();
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"WiFi Scan completed", "in "+(std::string)(_lastscan - _startscan)+" ms");
      #endif
      return true;
    }
    else {
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"Scan is cached",(std::string)(millis()-_lastscan )+" ms ago");
      #endif
    }
    return false;
}

std::string WiFiManager::WiFiManager::getScanItemOut(){
    std::string page;

    if(!_numNetworks) WiFi_scanNetworks(); // scan in case this gets called before any scans

    int n = _numNetworks;
    if (n == 0) {
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM("No networks found");
      #endif
      page += S_nonetworks; // @token nonetworks
      page += "<br/><br/>";
    }
    else {
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(n,"networks found");
      #endif
      //sort networks
      int indices[n];
      for (int i = 0; i < n; i++) {
        indices[i] = i;
      }

      // RSSI SORT
      for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
          if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
            std::swap(indices[i], indices[j]);
          }
        }
      }

      /* test std:sort
        std::sort(indices, indices + n, [](const int & a, const int & b) -> bool
        {
        return WiFi.RSSI(a) > WiFi.RSSI(b);
        });
       */

      // remove duplicates ( must be RSSI sorted )
      if (_removeDuplicateAPs) {
        std::string cssid;
        for (int i = 0; i < n; i++) {
          if (indices[i] == -1) continue;
          cssid = WiFi.SSID(indices[i]);
          for (int j = i + 1; j < n; j++) {
            if (cssid == WiFi.SSID(indices[j])) {
              #ifdef WM_DEBUG_LEVEL
              DEBUG_WM(WM_DEBUG_VERBOSE,"DUP AP:",WiFi.SSID(indices[j]));
              #endif
              indices[j] = -1; // set dup aps to index -1
            }
          }
        }
      }

      // token precheck, to speed up replacements on large ap lists
      std::string HTTP_ITEM_STR = HTTP_ITEM;

      // toggle icons with percentage
      HTTP_ITEM_STR.replace("{qp}", HTTP_ITEM_QP);
      HTTP_ITEM_STR.replace("{h}",_scanDispOptions ? "" : "h");
      HTTP_ITEM_STR.replace("{qi}", HTTP_ITEM_QI);
      HTTP_ITEM_STR.replace("{h}",_scanDispOptions ? "h" : "");
 
      // set token precheck flags
      bool tok_r = HTTP_ITEM_STR.indexOf(T_r) > 0;
      bool tok_R = HTTP_ITEM_STR.indexOf(T_R) > 0;
      bool tok_e = HTTP_ITEM_STR.indexOf(T_e) > 0;
      bool tok_q = HTTP_ITEM_STR.indexOf(T_q) > 0;
      bool tok_i = HTTP_ITEM_STR.indexOf(T_i) > 0;
      
      //display networks in page
      for (int i = 0; i < n; i++) {
        if (indices[i] == -1) continue; // skip dups

        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_VERBOSE,"AP: ",(std::string)WiFi.RSSI(indices[i]) + " " + (std::string)WiFi.SSID(indices[i]));
        #endif

        int rssiperc = getRSSIasQuality(WiFi.RSSI(indices[i]));
        uint8_t enc_type = WiFi.encryptionType(indices[i]);

        if (_minimumQuality == -1 || _minimumQuality < rssiperc) {
          std::string item = HTTP_ITEM_STR;
          if(WiFi.SSID(indices[i]) == ""){
            // Serial.println(WiFi.BSSIDstr(indices[i]));
            continue; // No idea why I am seeing these, lets just skip them for now
          }
          item.replace(T_V, htmlEntities(WiFi.SSID(indices[i]))); // ssid no encoding
          item.replace(T_v, htmlEntities(WiFi.SSID(indices[i]),true)); // ssid no encoding
          if(tok_e) item.replace(T_e, encryptionTypeStr(enc_type));
          if(tok_r) item.replace(T_r, (std::string)rssiperc); // rssi percentage 0-100
          if(tok_R) item.replace(T_R, (std::string)WiFi.RSSI(indices[i])); // rssi db
          if(tok_q) item.replace(T_q, (std::string)int(round(map(rssiperc,0,100,1,4)))); //quality icon 1-4
          if(tok_i){
            if (enc_type != WM_WIFIOPEN) {
              item.replace(T_i, "l");
            } else {
              item.replace(T_i, "");
            }
          }
          #ifdef WM_DEBUG_LEVEL
          DEBUG_WM(WM_DEBUG_DEV,item);
          #endif
          page += item;
          delay(0);
        } else {
          #ifdef WM_DEBUG_LEVEL
          DEBUG_WM(WM_DEBUG_VERBOSE,"Skipping , does not meet _minimumQuality");
          #endif
        }

      }
      page += HTTP_BR;
    }

    return page;
}

std::string WiFiManager::getIpForm(std::string id, std::string title, std::string value){
    std::string item = HTTP_FORM_LABEL;
    item += HTTP_FORM_PARAM;
    item.replace(T_i, id);
    item.replace(T_n, id);
    item.replace(T_p, T_t);
    // item.replace(T_p, default);
    item.replace(T_t, title);
    item.replace(T_l, "15");
    item.replace(T_v, value);
    item.replace(T_c, "");
    return item;  
}

std::string WiFiManager::getStaticOut(){
  std::string page;
  if ((_staShowStaticFields || _sta_static_ip) && _staShowStaticFields>=0) {
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_DEV,"_staShowStaticFields");
    #endif
    page += HTTP_FORM_STATIC_HEAD;
    // @todo how can we get these accurate settings from memory , wifi_get_ip_info does not seem to reveal if struct ip_info is static or not
    page += getIpForm(S_ip,S_staticip,(_sta_static_ip ? _sta_static_ip.toString() : "")); // @token staticip
    // WiFi.localIP().toString();
    page += getIpForm(S_gw,S_staticgw,(_sta_static_gw ? _sta_static_gw.toString() : "")); // @token staticgw
    // WiFi.gatewayIP().toString();
    page += getIpForm(S_sn,S_subnet,(_sta_static_sn ? _sta_static_sn.toString() : "")); // @token subnet
    // WiFi.subnetMask().toString();
  }

  if((_staShowDns || _sta_static_dns) && _staShowDns>=0){
    page += getIpForm(S_dns,S_staticdns,(_sta_static_dns ? _sta_static_dns.toString() : "")); // @token dns
  }

  if(page!="") page += HTTP_BR; // @todo remove these, use css

  return page;
}

std::string WiFiManager::getParamOut(){
  std::string page;

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV,"getParamOut",_paramsCount);
  #endif

  if(_paramsCount > 0){

    std::string HTTP_PARAM_temp = HTTP_FORM_LABEL;
    HTTP_PARAM_temp += HTTP_FORM_PARAM;
    bool tok_I = HTTP_PARAM_temp.indexOf(T_I) > 0;
    bool tok_i = HTTP_PARAM_temp.indexOf(T_i) > 0;
    bool tok_n = HTTP_PARAM_temp.indexOf(T_n) > 0;
    bool tok_p = HTTP_PARAM_temp.indexOf(T_p) > 0;
    bool tok_t = HTTP_PARAM_temp.indexOf(T_t) > 0;
    bool tok_l = HTTP_PARAM_temp.indexOf(T_l) > 0;
    bool tok_v = HTTP_PARAM_temp.indexOf(T_v) > 0;
    bool tok_c = HTTP_PARAM_temp.indexOf(T_c) > 0;

    char valLength[5];

    for (int i = 0; i < _paramsCount; i++) {
      //Serial.println((std::string)_params[i]->_length);
      if (_params[i] == NULL || _params[i]->_length > 99999) {
        // try to detect param scope issues, doesnt always catch but works ok
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] WiFiManagerParameter is out of scope");
        #endif
        return "";
      }
    }

    // add the extra parameters to the form
    for (int i = 0; i < _paramsCount; i++) {
     // label before or after, @todo this could be done via floats or CSS and eliminated
     std::string pitem;
      switch (_params[i]->getLabelPlacement()) {
        case WFM_LABEL_BEFORE:
          pitem = HTTP_FORM_LABEL;
          pitem += HTTP_FORM_PARAM;
          break;
        case WFM_LABEL_AFTER:
          pitem = HTTP_FORM_PARAM;
          pitem += HTTP_FORM_LABEL;
          break;
        default:
          // WFM_NO_LABEL
          pitem = HTTP_FORM_PARAM;
          break;
      }

      // Input templating
      // "<br/><input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>";
      // if no ID use customhtml for item, else generate from param string
      if (_params[i]->getID() != NULL) {
        if(tok_I)pitem.replace(T_I, (std::string)S_parampre+(std::string)i); // T_I id number
        if(tok_i)pitem.replace(T_i, _params[i]->getID()); // T_i id name
        if(tok_n)pitem.replace(T_n, _params[i]->getID()); // T_n id name alias
        if(tok_p)pitem.replace(T_p, T_t); // T_p replace legacy placeholder token
        if(tok_t)pitem.replace(T_t, _params[i]->getLabel()); // T_t title/label
        snprintf(valLength, 5, "%d", _params[i]->getValueLength());
        if(tok_l)pitem.replace(T_l, valLength); // T_l value length
        if(tok_v)pitem.replace(T_v, _params[i]->getValue()); // T_v value
        if(tok_c)pitem.replace(T_c, _params[i]->getCustomHTML()); // T_c meant for additional attributes, not html, but can stuff
      } else {
        pitem = _params[i]->getCustomHTML();
      }

      page += pitem;
    }
  }

  return page;
}

void WiFiManager::handleWiFiStatus(){
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"<- HTTP WiFi status ");
  #endif
  handleRequest();
  std::string page;
  // std::string page = "{\"result\":true,\"count\":1}";
  #ifdef WM_JSTEST
    page = HTTP_JS;
  #endif
  HTTPSend(page);
}

/** 
 * HTTPD CALLBACK save form and redirect to WLAN config page again
 */
void WiFiManager::handleWifiSave() {
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"<- HTTP WiFi save ");
  DEBUG_WM(WM_DEBUG_DEV,"Method:",server->method() == HTTP_GET  ? (std::string)S_GET : (std::string)S_POST);
  #endif
  handleRequest();

  //SAVE/connect here
  _ssid = server->arg("s").c_str();
  _pass = server->arg("p").c_str();

  if(_ssid == "" && _pass != ""){
    _ssid = WiFi_SSID(true); // password change, placeholder ssid, @todo compare pass to old?, confirm ssid is clean
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"Detected WiFi password change");
    #endif    
  }

  #ifdef WM_DEBUG_LEVEL
  std::string requestinfo = "SERVER_REQUEST\n----------------\n";
  requestinfo += "URI: ";
  requestinfo += server->uri();
  requestinfo += "\nMethod: ";
  requestinfo += (server->method() == HTTP_GET) ? "GET" : "POST";
  requestinfo += "\nArguments: ";
  requestinfo += server->args();
  requestinfo += "\n";
  for (uint8_t i = 0; i < server->args(); i++) {
    requestinfo += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }

  DEBUG_WM(WM_DEBUG_MAX,requestinfo);
  #endif

  // set static ips from server args
  if (server->arg(S_ip) != "") {
    //_sta_static_ip.fromString(server->arg(S_ip));
    std::string ip = server->arg(S_ip);
    optionalIPFromString(&_sta_static_ip, ip.c_str());
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_DEV,"static ip:",ip);
    #endif
  }
  if (server->arg(S_gw) != "") {
    std::string gw = server->arg(S_gw);
    optionalIPFromString(&_sta_static_gw, gw.c_str());
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_DEV,"static gateway:",gw);
    #endif
  }
  if (server->arg(S_sn) != "") {
    std::string sn = server->arg(S_sn);
    optionalIPFromString(&_sta_static_sn, sn.c_str());
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_DEV,"static netmask:",sn);
    #endif
  }
  if (server->arg(S_dns) != "") {
    std::string dns = server->arg(S_dns);
    optionalIPFromString(&_sta_static_dns, dns.c_str());
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_DEV,"static DNS:",dns);
    #endif
  }

  if (_presavewificallback != NULL) {
    _presavewificallback();  // @CALLBACK 
  }

  if(_paramsInWifi) doParamSave();

  std::string page;

  if(_ssid == ""){
    page = getHTTPHead(S_titlewifisettings, C_wifi); // @token titleparamsaved
    page += HTTP_PARAMSAVED;
  }
  else {
    page = getHTTPHead(S_titlewifisaved, C_wifi); // @token titlewifisaved
    page += HTTP_SAVED;
  }

  if(_showBack) page += HTTP_BACKBTN;
  page += getHTTPEnd();

  server->sendHeader(HTTP_HEAD_CORS, HTTP_HEAD_CORS_ALLOW_ALL); // @HTTPHEAD send cors
  HTTPSend(page);

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV,"Sent wifi save page");
  #endif

  connect = true; //signal ready to connect/reset process in processConfigPortal
}

void WiFiManager::handleParamSave() {

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"<- HTTP Param save ");
  #endif
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV,"Method:",server->method() == HTTP_GET  ? (std::string)S_GET : (std::string)S_POST);
  #endif
  handleRequest();

  doParamSave();

  std::string page = getHTTPHead(S_titleparamsaved, C_param); // @token titleparamsaved
  page += HTTP_PARAMSAVED;
  if(_showBack) page += HTTP_BACKBTN; 
  page += getHTTPEnd();

  HTTPSend(page);

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV,"Sent param save page");
  #endif
}

void WiFiManager::doParamSave(){
   // @todo use new callback for before paramsaves, is this really needed?
  if ( _presaveparamscallback != NULL) {
    _presaveparamscallback();  // @CALLBACK
  }

  //parameters
  if(_paramsCount > 0){
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"Parameters");
    DEBUG_WM(WM_DEBUG_VERBOSE,FPSTR(D_HR));
    #endif

    for (int i = 0; i < _paramsCount; i++) {
      if (_params[i] == NULL || _params[i]->_length > 99999) {
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] WiFiManagerParameter is out of scope");
        #endif
        break; // @todo might not be needed anymore
      }
      //read parameter from server
      std::string name = (std::string)S_parampre+(std::string)i;
      std::string value;
      if(server->hasArg(name)) {
        value = server->arg(name);
      } else {
        value = server->arg(_params[i]->getID());
      }

      //store it in params array
      value.toCharArray(_params[i]->_value, _params[i]->_length+1); // length+1 null terminated
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,(std::string)_params[i]->getID() + ":",value);
      #endif
    }
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,FPSTR(D_HR));
    #endif
  }

   if ( _saveparamscallback != NULL) {
    _saveparamscallback();  // @CALLBACK
  }
   
}

/** 
 * HTTPD CALLBACK info page
 */
void WiFiManager::handleInfo() {
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"<- HTTP Info");
  #endif
  handleRequest();
  std::string page = getHTTPHead(S_titleinfo, C_info); // @token titleinfo
  reportStatus(page);

  uint16_t infos = 0;

  //@todo convert to enum or refactor to strings
  //@todo wrap in build flag to remove all info code for memory saving
  #ifdef ESP8266
    infos = 28;
    std::string infoids[] = {
      "esphead",
      "uptime",
      "chipid",
      "fchipid",
      "idesize",
      "flashsize",
      "corever",
      "bootver",
      "cpufreq",
      "freeheap",
      "memsketch",
      "memsmeter",
      "lastreset",
      "wifihead",
      "conx",
      "stassid",
      "staip",
      "stagw",
      "stasub",
      "dnss",
      "host",
      "stamac",
      "autoconx",
      "wifiaphead",
      "apssid",
      "apip",
      "apbssid",
      "apmac"
    };

  #elif defined(ESP32)
    // add esp_chip_info ?
    infos = 27;
    std::string infoids[] = {
      "esphead",
      "uptime",
      "chipid",
      "chiprev",
      "idesize",
      "flashsize",      
      "cpufreq",
      "freeheap",
      "memsketch",
      "memsmeter",      
      "lastreset",
      "temp",
      // "hall",
      "wifihead",
      "conx",
      "stassid",
      "staip",
      "stagw",
      "stasub",
      "dnss",
      "host",
      "stamac",
      "apssid",
      "wifiaphead",
      "apip",
      "apmac",
      "aphost",
      "apbssid"
    };
  #endif

  for(size_t i=0; i<infos;i++){
    if(infoids[i] != NULL) page += getInfoData(infoids[i]);
  }
  page += "</dl>";

  page += "<h3>About</h3><hr><dl>";
  page += getInfoData("aboutver");
  page += getInfoData("aboutarduinover");
  page += getInfoData("aboutidfver");
  page += getInfoData("aboutdate");
  page += "</dl>";

  if(_showInfoUpdate){
    page += HTTP_PORTAL_MENU[8];
    page += HTTP_PORTAL_MENU[9];
  }
  if(_showInfoErase) page += HTTP_ERASEBTN;
  if(_showBack) page += HTTP_BACKBTN;
  page += HTTP_HELP;
  page += getHTTPEnd();

  HTTPSend(page);

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV,"Sent info page");
  #endif
}

std::string WiFiManager::getInfoData(std::string id){

  std::string p;
  if(id=="esphead"){
    p = HTTP_INFO_esphead;
    #ifdef ESP32
      p.replace(T_1, (std::string)ESP.getChipModel());
    #endif
  }
  else if(id=="wifihead"){
    p = HTTP_INFO_wifihead;
    p.replace(T_1,getModeString(WiFi.getMode()));
  }
  else if(id=="uptime"){
    // subject to rollover!
    p = HTTP_INFO_uptime;
    p.replace(T_1,(std::string)(millis() / 1000 / 60));
    p.replace(T_2,(std::string)((millis() / 1000) % 60));
  }
  else if(id=="chipid"){
    p = HTTP_INFO_chipid;
    p.replace(T_1,std::string(WIFI_getChipId(),HEX));
  }
  #ifdef ESP32
  else if(id=="chiprev"){
      p = HTTP_INFO_chiprev;
      std::string rev = (std::string)ESP.getChipRevision();
      #ifdef _SOC_EFUSE_REG_H_
        std::string revb = (std::string)(REG_READ(EFUSE_BLK0_RDATA3_REG) >> (EFUSE_RD_CHIP_VER_RESERVE_S)&&EFUSE_RD_CHIP_VER_RESERVE_V);
        p.replace(T_1,rev+"<br/>"+revb);
      #else
        p.replace(T_1,rev);
      #endif
  }
  #endif
  #ifdef ESP8266
  else if(id=="fchipid"){
      p = HTTP_INFO_fchipid;
      p.replace(T_1,(std::string)ESP.getFlashChipId());
  }
  #endif
  else if(id=="idesize"){
    p = HTTP_INFO_idesize;
    p.replace(T_1,(std::string)ESP.getFlashChipSize());
  }
  else if(id=="flashsize"){
    #ifdef ESP8266
      p = HTTP_INFO_flashsize;
      p.replace(T_1,(std::string)ESP.getFlashChipRealSize());
    #elif defined ESP32
      p = HTTP_INFO_psrsize;
      p.replace(T_1,(std::string)ESP.getPsramSize());      
    #endif
  }
  else if(id=="corever"){
    #ifdef ESP8266
      p = HTTP_INFO_corever;
      p.replace(T_1,(std::string)ESP.getCoreVersion());
    #endif      
  }
  #ifdef ESP8266
  else if(id=="bootver"){
      p = HTTP_INFO_bootver;
      p.replace(T_1,(std::string)system_get_boot_version());
  }
  #endif
  else if(id=="cpufreq"){
    p = HTTP_INFO_cpufreq;
    p.replace(T_1,(std::string)ESP.getCpuFreqMHz());
  }
  else if(id=="freeheap"){
    p = HTTP_INFO_freeheap;
    p.replace(T_1,(std::string)ESP.getFreeHeap());
  }
  else if(id=="memsketch"){
    p = HTTP_INFO_memsketch;
    p.replace(T_1,(std::string)(ESP.getSketchSize()));
    p.replace(T_2,(std::string)(ESP.getSketchSize()+ESP.getFreeSketchSpace()));
  }
  else if(id=="memsmeter"){
    p = HTTP_INFO_memsmeter;
    p.replace(T_1,(std::string)(ESP.getSketchSize()));
    p.replace(T_2,(std::string)(ESP.getSketchSize()+ESP.getFreeSketchSpace()));
  }
  else if(id=="lastreset"){
    #ifdef ESP8266
      p = HTTP_INFO_lastreset;
      p.replace(T_1,(std::string)ESP.getResetReason());
    #elif defined(ESP32) && defined(_ROM_RTC_H_)
      // requires #include <rom/rtc.h>
      p = HTTP_INFO_lastreset;
      for(int i=0;i<2;i++){
        int reason = rtc_get_reset_reason(i);
        std::string tok = (std::string)T_ss+(std::string)(i+1)+(std::string)T_es;
        switch (reason)
        {
          //@todo move to array
          case 1  : p.replace(tok,"Vbat power on reset");break;
          case 3  : p.replace(tok,"Software reset digital core");break;
          case 4  : p.replace(tok,"Legacy watch dog reset digital core");break;
          case 5  : p.replace(tok,"Deep Sleep reset digital core");break;
          case 6  : p.replace(tok,"Reset by SLC module, reset digital core");break;
          case 7  : p.replace(tok,"Timer Group0 Watch dog reset digital core");break;
          case 8  : p.replace(tok,"Timer Group1 Watch dog reset digital core");break;
          case 9  : p.replace(tok,"RTC Watch dog Reset digital core");break;
          case 10 : p.replace(tok,"Instrusion tested to reset CPU");break;
          case 11 : p.replace(tok,"Time Group reset CPU");break;
          case 12 : p.replace(tok,"Software reset CPU");break;
          case 13 : p.replace(tok,"RTC Watch dog Reset CPU");break;
          case 14 : p.replace(tok,"for APP CPU, reseted by PRO CPU");break;
          case 15 : p.replace(tok,"Reset when the vdd voltage is not stable");break;
          case 16 : p.replace(tok,"RTC Watch dog reset digital core and rtc module");break;
          default : p.replace(tok,"NO_MEAN");
        }
      }
    #endif
  }
  else if(id=="apip"){
    p = HTTP_INFO_apip;
    p.replace(T_1,WiFi.softAPIP().toString());
  }
  else if(id=="apmac"){
    p = HTTP_INFO_apmac;
    p.replace(T_1,(std::string)WiFi.softAPmacAddress());
  }
  #ifdef ESP32
  else if(id=="aphost"){
      p = HTTP_INFO_aphost;
      p.replace(T_1,WiFi.softAPgetHostname());
  }
  #endif
  #ifndef WM_NOSOFTAPSSID
  #ifdef ESP8266
  else if(id=="apssid"){
    p = HTTP_INFO_apssid;
    p.replace(T_1,htmlEntities(WiFi.softAPSSID()));
  }
  #endif
  #endif
  else if(id=="apbssid"){
    p = HTTP_INFO_apbssid;
    p.replace(T_1,(std::string)WiFi.BSSIDstr());
  }
  // softAPgetHostname // esp32
  // softAPSubnetCIDR
  // softAPNetworkID
  // softAPBroadcastIP

  else if(id=="stassid"){
    p = HTTP_INFO_stassid;
    p.replace(T_1,htmlEntities((std::string)WiFi_SSID()));
  }
  else if(id=="staip"){
    p = HTTP_INFO_staip;
    p.replace(T_1,WiFi.localIP().toString());
  }
  else if(id=="stagw"){
    p = HTTP_INFO_stagw;
    p.replace(T_1,WiFi.gatewayIP().toString());
  }
  else if(id=="stasub"){
    p = HTTP_INFO_stasub;
    p.replace(T_1,WiFi.subnetMask().toString());
  }
  else if(id=="dnss"){
    p = HTTP_INFO_dnss;
    p.replace(T_1,WiFi.dnsIP().toString());
  }
  else if(id=="host"){
    p = HTTP_INFO_host;
    #ifdef ESP32
      p.replace(T_1,WiFi.getHostname());
    #else
    p.replace(T_1,WiFi.hostname());
    #endif
  }
  else if(id=="stamac"){
    p = HTTP_INFO_stamac;
    p.replace(T_1,WiFi.macAddress());
  }
  else if(id=="conx"){
    p = HTTP_INFO_conx;
    p.replace(T_1,WiFi.isConnected() ? S_y : S_n);
  }
  #ifdef ESP8266
  else if(id=="autoconx"){
    p = HTTP_INFO_autoconx;
    p.replace(T_1,WiFi.getAutoConnect() ? S_enable : S_disable);
  }
  #endif
  #if defined(ESP32) && !defined(WM_NOTEMP)
  else if(id=="temp"){
    // temperature is not calibrated, varying large offsets are present, use for relative temp changes only
    p = HTTP_INFO_temp;
    p.replace(T_1,(std::string)temperatureRead());
    p.replace(T_2,(std::string)((temperatureRead()+32)*1.8f));
  }
  // else if(id=="hall"){ 
  //   p = HTTP_INFO_hall;
  //   p.replace(T_1,(std::string)hallRead()); // hall sensor reads can cause issues with adcs
  // }
  #endif
  else if(id=="aboutver"){
    p = HTTP_INFO_aboutver;
    p.replace(T_1,WM_VERSION_STR);
  }
  else if(id=="aboutarduinover"){
    #ifdef VER_ARDUINO_STR
    p = HTTP_INFO_aboutarduino;
    p.replace(T_1,std::string(VER_ARDUINO_STR));
    #endif
  }
  // else if(id=="aboutidfver"){
  //   #ifdef VER_IDF_STR
  //   p = HTTP_INFO_aboutidf;
  //   p.replace(T_1,std::string(VER_IDF_STR));
  //   #endif
  // }
  else if(id=="aboutsdkver"){
    p = HTTP_INFO_sdkver;
    #ifdef ESP32
      p.replace(T_1,(std::string)esp_get_idf_version());
      // p.replace(T_1,(std::string)system_get_sdk_version()); // deprecated
    #else
    p.replace(T_1,(std::string)system_get_sdk_version());
    #endif
  }
  else if(id=="aboutdate"){
    p = HTTP_INFO_aboutdate;
    p.replace(T_1,std::string(__DATE__ " " __TIME__));
  }
  return p;
}

/** 
 * HTTPD CALLBACK exit, closes configportal if blocking, if non blocking undefined
 */
void WiFiManager::handleExit() {
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"<- HTTP Exit");
  #endif
  handleRequest();
  std::string page = getHTTPHead(S_titleexit, C_exit); // @token titleexit
  page += S_exiting; // @token exiting
  page += getHTTPEnd();
  // ('Logout', 401, {'WWW-Authenticate': 'Basic realm="Login required"'})
  server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); // @HTTPHEAD send cache
  HTTPSend(page);
  delay(2000);
  abort = true;
}

/** 
 * HTTPD CALLBACK reset page
 */
void WiFiManager::handleReset() {
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"<- HTTP Reset");
  #endif
  handleRequest();
  std::string page = getHTTPHead(S_titlereset, C_restart); //@token titlereset
  page += S_resetting; //@token resetting
  page += getHTTPEnd();

  HTTPSend(page);

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM("RESETTING ESP");
  #endif
  delay(1000);
  reboot();
}

/** 
 * HTTPD CALLBACK erase page
 */

// void WiFiManager::handleErase() {
//   handleErase(false);
// }
void WiFiManager::handleErase(boolean opt) {
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_NOTIFY,"<- HTTP Erase");
  #endif
  handleRequest();
  std::string page = getHTTPHead(S_titleerase, C_erase); // @token titleerase

  bool ret = erase(opt);

  if(ret) page += S_resetting; // @token resetting
  else {
    page += S_error; // @token erroroccur
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] WiFi EraseConfig failed");
    #endif
  }

  page += getHTTPEnd();
  HTTPSend(page);

  if(ret){
    delay(2000);
    #ifdef WM_DEBUG_LEVEL
  	DEBUG_WM("RESETTING ESP");
    #endif
  	reboot();
  }	
}

/** 
 * HTTPD CALLBACK 404
 */
void WiFiManager::handleNotFound() {
  if (captivePortal()) return; // If captive portal redirect instead of displaying the page
  handleRequest();
  std::string message = S_notfound; // @token notfound

  bool verbose404 = false; // show info in 404 body, uri,method, args
  if(verbose404){
    message += S_uri; // @token uri
    message += server->uri();
    message += S_method; // @token method
    message += ( server->method() == HTTP_GET ) ? S_GET : S_POST;
    message += S_args; // @token args
    message += server->args();
    message += "\n";

    for ( uint8_t i = 0; i < server->args(); i++ ) {
      message += " " + server->argName ( i ) + ": " + server->arg ( i ) + "\n";
    }
  }
  server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); // @HTTPHEAD send cache
  server->sendHeader("Pragma", "no-cache");
  server->sendHeader("Expires", "-1");
  server->send ( 404, HTTP_HEAD_CT2, message );
}

/**
 * HTTPD redirector
 * Redirect to captive portal if we got a request for another domain. 
 * Return true in that case so the page handler do not try to handle the request again. 
 */
boolean WiFiManager::captivePortal() {
  
  if(!_enableCaptivePortal || !configPortalActive) return false; // skip redirections if cp not enabled or not in ap mode
  
  std::string serverLoc =  toStringIp(server->client().localIP());

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV,"-> " + server->hostHeader());
  DEBUG_WM(WM_DEBUG_DEV,"serverLoc " + serverLoc);
  #endif

  // fallback for ipv6 bug
  if(serverLoc == "0.0.0.0"){
    if ((WiFi.status()) != WL_CONNECTED)
      serverLoc = toStringIp(WiFi.softAPIP());
    else
      serverLoc = toStringIp(WiFi.localIP());
  }
  
  if(_httpPort != 80) serverLoc += ":" + (std::string)_httpPort; // add port if not default
  bool doredirect = serverLoc != server->hostHeader(); // redirect if hostheader not server ip, prevent redirect loops
  
  if (doredirect) {
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"<- Request redirected to captive portal");
    DEBUG_WM(WM_DEBUG_DEV,"serverLoc " + serverLoc);
    #endif
    server->sendHeader("Location", (std::string)"http://" + serverLoc, true); // @HTTPHEAD send redirect
    server->send ( 302, HTTP_HEAD_CT2, ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server->client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

void WiFiManager::stopCaptivePortal(){
  _enableCaptivePortal= false;
  // @todo maybe disable configportaltimeout(optional), or just provide callback for user
}

// HTTPD CALLBACK, handle close,  stop captive portal, if not enabled undefined
void WiFiManager::handleClose(){
  DEBUG_WM(WM_DEBUG_VERBOSE,"Disabling Captive Portal");
  stopCaptivePortal();
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"<- HTTP close");
  #endif
  handleRequest();
  std::string page = getHTTPHead(S_titleclose, C_close); // @token titleclose
  page += S_closing; // @token closing
  page += getHTTPEnd();
  HTTPSend(page);
}

void WiFiManager::reportStatus(std::string &page){
  // updateConxResult(WiFi.status()); // @todo: this defeats the purpose of last result, update elsewhere or add logic here
  DEBUG_WM(WM_DEBUG_DEV,"[WIFI] reportStatus prev:",getWLStatusString(_lastconxresult));
  DEBUG_WM(WM_DEBUG_DEV,"[WIFI] reportStatus current:",getWLStatusString(WiFi.status()));
  std::string str;
  if (WiFi_SSID() != ""){
    if (WiFi.status()==WL_CONNECTED){
      str = HTTP_STATUS_ON;
      str.replace(T_i,WiFi.localIP().toString());
      str.replace(T_v,htmlEntities(WiFi_SSID()));
    }
    else {
      str = HTTP_STATUS_OFF;
      str.replace(T_v,htmlEntities(WiFi_SSID()));
      if(_lastconxresult == WL_STATION_WRONG_PASSWORD){
        // wrong password
        str.replace(T_c,"D"); // class
        str.replace(T_r,HTTP_STATUS_OFFPW);
      }
      else if(_lastconxresult == WL_NO_SSID_AVAIL){
        // connect failed, or ap not found
        str.replace(T_c,"D");
        str.replace(T_r,HTTP_STATUS_OFFNOAP);
      }
      else if(_lastconxresult == WL_CONNECT_FAILED){
        // connect failed
        str.replace(T_c,"D");
        str.replace(T_r,HTTP_STATUS_OFFFAIL);
      }
      else if(_lastconxresult == WL_CONNECTION_LOST){
        // connect failed, MOST likely 4WAY_HANDSHAKE_TIMEOUT/incorrect password, state is ambiguous however
        str.replace(T_c,"D");
        str.replace(T_r,HTTP_STATUS_OFFFAIL);
      }
      else{
        str.replace(T_c,"");
        str.replace(T_r,"");
      } 
    }
  }
  else {
    str = HTTP_STATUS_NONE;
  }
  page += str;
}

// PUBLIC

// METHODS

/**
 * reset wifi settings, clean stored ap password
 */

/**
 * [stopConfigPortal description]
 * @return {[type]} [description]
 */
bool WiFiManager::stopConfigPortal(){
  if(_configPortalIsBlocking){
    abort = true;
    return true;
  }
  return shutdownConfigPortal();  
}

/**
 * disconnect
 * @access public
 * @since $dev
 * @return bool success
 */
bool WiFiManager::disconnect(){
  if(WiFi.status() != WL_CONNECTED){
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"Disconnecting: Not connected");
    #endif
    return false;
  }  
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM("Disconnecting");
  #endif
  return WiFi_Disconnect();
}

/**
 * reboot the device
 * @access public
 */
void WiFiManager::reboot(){
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM("Restarting");
  #endif
  ESP.restart();
}

/**
 * reboot the device
 * @access public
 */
bool WiFiManager::erase(){
  return erase(false);
}

bool WiFiManager::erase(bool opt){
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM("Erasing");
  #endif

  #if defined(ESP32) && ((defined(WM_ERASE_NVS) || defined(nvs_flash_h)))
    // if opt true, do nvs erase
    if(opt){
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM("Erasing NVS");
      #endif
      esp_err_t err;
      err = nvs_flash_init();
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"nvs_flash_init: ",err!=ESP_OK ? (std::string)err : "Success");
      #endif
      err = nvs_flash_erase();
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"nvs_flash_erase: ", err!=ESP_OK ? (std::string)err : "Success");
      #endif
      return err == ESP_OK;
    }
  #elif defined(ESP8266) && defined(spiffs_api_h)
    if(opt){
      bool ret = false;
      if(SPIFFS.begin()){
      #ifdef WM_DEBUG_LEVEL
        DEBUG_WM("Erasing SPIFFS");
        #endif
        bool ret = SPIFFS.format();
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_VERBOSE,"spiffs erase: ",ret ? "Success" : "ERROR");
        #endif
      } else{
      #ifdef WM_DEBUG_LEVEL
        DEBUG_WM("[ERROR] Could not start SPIFFS");
        #endif
      }
      return ret;
    }
  #else
    (void)opt;
  #endif

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM("Erasing WiFi Config");
  #endif
  return WiFi_eraseConfig();
}

/**
 * [resetSettings description]
 * ERASES STA CREDENTIALS
 * @access public
 */
void WiFiManager::resetSettings() {
#ifdef WM_DEBUG_LEVEL
  DEBUG_WM("resetSettings");
  #endif
  WiFi_enableSTA(true,true); // must be sta to disconnect erase
  delay(500); // ensure sta is enabled
  if (_resetcallback != NULL){
      _resetcallback();  // @CALLBACK
  }
  
  #ifdef ESP32
    WiFi.disconnect(true,true);
  #else
    WiFi.persistent(true);
    WiFi.disconnect(true);
    WiFi.persistent(false);
  #endif
  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM("SETTINGS ERASED");
  #endif
}

// SETTERS

/**
 * [setTimeout description]
 * @access public
 * @param {[type]} unsigned long seconds [description]
 */
void WiFiManager::setTimeout(unsigned long seconds) {
  setConfigPortalTimeout(seconds);
}

/**
 * [setConfigPortalTimeout description]
 * @access public
 * @param {[type]} unsigned long seconds [description]
 */
void WiFiManager::setConfigPortalTimeout(unsigned long seconds) {
  _configPortalTimeout = seconds * 1000;
}

/**
 * [setConnectTimeout description]
 * @access public
 * @param {[type]} unsigned long seconds [description]
 */
void WiFiManager::setConnectTimeout(unsigned long seconds) {
  _connectTimeout = seconds * 1000;
}

/**
 * [setConnectRetries description]
 * @access public
 * @param {[type]} uint8_t numRetries [description]
 */
void WiFiManager::setConnectRetries(uint8_t numRetries){
  _connectRetries = constrain(numRetries,1,10);
}

/**
 * toggle _cleanconnect, always disconnect before connecting
 * @param {[type]} bool enable [description]
 */
void WiFiManager::setCleanConnect(bool enable){
  _cleanConnect = enable;
}

/**
 * [setConnectTimeout description
 * @access public
 * @param {[type]} unsigned long seconds [description]
 */
void WiFiManager::setSaveConnectTimeout(unsigned long seconds) {
  _saveTimeout = seconds * 1000;
}

/**
 * Set save portal connect on save option, 
 * if false, will only save credentials not connect
 * @access public
 * @param {[type]} bool connect [description]
 */
void WiFiManager::setSaveConnect(bool connect) {
  _connectonsave = connect;
}

/**
 * [setDebugOutput description]
 * @access public
 * @param {[type]} boolean debug [description]
 */
void WiFiManager::setDebugOutput(boolean debug) {
  _debug = debug;
  if(_debug && _debugLevel == WM_DEBUG_DEV) debugPlatformInfo();
  if(_debug && _debugLevel >= WM_DEBUG_NOTIFY)DEBUG_WM((__FlashStringHelper *)WM_VERSION_STR," D:"+std::string(_debugLevel));
}

void WiFiManager::setDebugOutput(boolean debug, std::string prefix) {
  _debugPrefix = prefix;
  setDebugOutput(debug);
}

void WiFiManager::setDebugOutput(boolean debug, wm_debuglevel_t level) {
  _debugLevel = level;
  // _debugPrefix = prefix;
  setDebugOutput(debug);
}


/**
 * [setAPStaticIPConfig description]
 * @access public
 * @param {[type]} IPAddress ip [description]
 * @param {[type]} IPAddress gw [description]
 * @param {[type]} IPAddress sn [description]
 */
void WiFiManager::setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn) {
  _ap_static_ip = ip;
  _ap_static_gw = gw;
  _ap_static_sn = sn;
}

/**
 * [setSTAStaticIPConfig description]
 * @access public
 * @param {[type]} IPAddress ip [description]
 * @param {[type]} IPAddress gw [description]
 * @param {[type]} IPAddress sn [description]
 */
void WiFiManager::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn) {
  _sta_static_ip = ip;
  _sta_static_gw = gw;
  _sta_static_sn = sn;
}

/**
 * [setSTAStaticIPConfig description]
 * @since $dev
 * @access public
 * @param {[type]} IPAddress ip [description]
 * @param {[type]} IPAddress gw [description]
 * @param {[type]} IPAddress sn [description]
 * @param {[type]} IPAddress dns [description]
 */
void WiFiManager::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns) {
  setSTAStaticIPConfig(ip,gw,sn);
  _sta_static_dns = dns;
}

/**
 * [setMinimumSignalQuality description]
 * @access public
 * @param {[type]} int quality [description]
 */
void WiFiManager::setMinimumSignalQuality(int quality) {
  _minimumQuality = quality;
}

/**
 * [setBreakAfterConfig description]
 * @access public
 * @param {[type]} boolean shouldBreak [description]
 */
void WiFiManager::setBreakAfterConfig(boolean shouldBreak) {
  _shouldBreakAfterConfig = shouldBreak;
}

/**
 * setAPCallback, set a callback when softap is started
 * @access public 
 * @param {[type]} void (*func)(WiFiManager* wminstance)
 */
void WiFiManager::setAPCallback( std::function<void(WiFiManager*)> func ) {
  _apcallback = func;
}

/**
 * setWebServerCallback, set a callback after webserver is reset, and before routes are setup
 * if we set webserver handlers before wm, they are used and wm is not by esp webserver
 * on events cannot be overrided once set, and are not mutiples
 * @access public 
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setWebServerCallback( std::function<void()> func ) {
  _webservercallback = func;
}

/**
 * setSaveConfigCallback, set a save config callback after closing configportal
 * @note calls only if wifi is saved or changed, or setBreakAfterConfig(true)
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setSaveConfigCallback( std::function<void()> func ) {
  _savewificallback = func;
}

/**
 * setPreSaveConfigCallback, set a callback to fire before saving wifi or params
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setPreSaveConfigCallback( std::function<void()> func ) {
  _presavewificallback = func;
}

/**
 * setConfigResetCallback, set a callback to occur when a resetSettings() occurs
 * @access public
 * @param {[type]} void(*func)(void)
 */
void WiFiManager::setConfigResetCallback( std::function<void()> func ) {
    _resetcallback = func;
}

/**
 * setSaveParamsCallback, set a save params callback on params save in wifi or params pages
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setSaveParamsCallback( std::function<void()> func ) {
  _saveparamscallback = func;
}

/**
 * setPreSaveParamsCallback, set a pre save params callback on params save prior to anything else
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setPreSaveParamsCallback( std::function<void()> func ) {
  _presaveparamscallback = func;
}

/**
 * setPreOtaUpdateCallback, set a callback to fire before OTA update
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setPreOtaUpdateCallback( std::function<void()> func ) {
  _preotaupdatecallback = func;
}

/**
 * setConfigPortalTimeoutCallback, set a callback to config portal is timeout
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setConfigPortalTimeoutCallback( std::function<void()> func ) {
  _configportaltimeoutcallback = func;
}

/**
 * set custom head html
 * custom element will be added to head, eg. new meta,style,script tag etc.
 * @access public
 * @param char element
 */
void WiFiManager::setCustomHeadElement(const char* html) {
  _customHeadElement = html;
}

/**
 * set custom html at the top of the body
 * custom element will be added after the body tag is opened, eg. to show a logo etc.
 * @access public
 * @param char element
 */
void WiFiManager::setCustomBodyHeader(const char* html) {
    _customBodyHeader = html;
}

/**
 * set custom html at the bottom of the body
 * custom element will be added before the body tag is closed
 * @access public
 * @param char element
 */
void WiFiManager::setCustomBodyFooter(const char* html) {
    _customBodyFooter = html;
}

/**
 * set custom menu html
 * custom element will be added to menu under custom menu item.
 * @access public
 * @param char element
 */
void WiFiManager::setCustomMenuHTML(const char* html) {
  _customMenuHTML = html;
}

/**
 * toggle wifiscan hiding of duplicate ssid names
 * if this is false, wifiscan will remove duplicat Access Points - defaut true
 * @access public
 * @param boolean removeDuplicates [true]
 */
void WiFiManager::setRemoveDuplicateAPs(boolean removeDuplicates) {
  _removeDuplicateAPs = removeDuplicates;
}

/**
 * toggle configportal blocking loop
 * if enabled, then the configportal will enter a blocking loop and wait for configuration
 * if disabled use with process() to manually process webserver
 * @since $dev
 * @access public
 * @param boolean shoudlBlock [false]
 */
void WiFiManager::setConfigPortalBlocking(boolean shouldBlock) {
  _configPortalIsBlocking = shouldBlock;
}

/**
 * toggle restore persistent, track internally
 * sets ESP wifi.persistent so we can remember it and restore user preference on destruct
 * there is no getter in esp8266 platform prior to https://github.com/esp8266/Arduino/pull/3857
 * @since $dev
 * @access public
 * @param boolean persistent [true]
 */
void WiFiManager::setRestorePersistent(boolean persistent) {
  _userpersistent = persistent;
  if(!persistent){
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM("persistent is off");
    #endif
  }
}

/**
 * toggle showing static ip form fields
 * if enabled, then the static ip, gateway, subnet fields will be visible, even if not set in code
 * @since $dev
 * @access public
 * @param boolean alwaysShow [false]
 */
void WiFiManager::setShowStaticFields(boolean alwaysShow){
  if(_disableIpFields) _staShowStaticFields = alwaysShow ? 1 : -1;
  else _staShowStaticFields = alwaysShow ? 1 : 0;
}

/**
 * toggle showing dns fields
 * if enabled, then the dns1 field will be visible, even if not set in code
 * @since $dev
 * @access public
 * @param boolean alwaysShow [false]
 */
void WiFiManager::setShowDnsFields(boolean alwaysShow){
  if(_disableIpFields) _staShowDns = alwaysShow ? 1 : -1;
  else _staShowDns = alwaysShow ? 1 : 0;
}

/**
 * toggle showing password in wifi password field
 * if not enabled, placeholder will be S_passph
 * @since $dev
 * @access public
 * @param boolean alwaysShow [false]
 */
void WiFiManager::setShowPassword(boolean show){
  _showPassword = show;
}

/**
 * toggle captive portal
 * if enabled, then devices that use captive portal checks will be redirected to root
 * if not you will automatically have to navigate to ip [192.168.4.1]
 * @since $dev
 * @access public
 * @param boolean enabled [true]
 */
void WiFiManager::setCaptivePortalEnable(boolean enabled){
  _enableCaptivePortal = enabled;
}

/**
 * toggle wifi autoreconnect policy
 * if enabled, then wifi will autoreconnect automatically always
 * On esp8266 we force this on when autoconnect is called, see notes
 * On esp32 this is handled on SYSTEM_EVENT_STA_DISCONNECTED since it does not exist in core yet
 * @since $dev
 * @access public
 * @param boolean enabled [true]
 */
void WiFiManager::setWiFiAutoReconnect(boolean enabled){
  _wifiAutoReconnect = enabled;
}

/**
 * toggle configportal timeout wait for station client
 * if enabled, then the configportal will start timeout when no stations are connected to softAP
 * disabled by default as rogue stations can keep it open if there is no auth
 * @since $dev
 * @access public
 * @param boolean enabled [false]
 */
void WiFiManager::setAPClientCheck(boolean enabled){
  _apClientCheck = enabled;
}

/**
 * toggle configportal timeout wait for web client
 * if enabled, then the configportal will restart timeout when client requests come in
 * @since $dev
 * @access public
 * @param boolean enabled [true]
 */
void WiFiManager::setWebPortalClientCheck(boolean enabled){
  _webClientCheck = enabled;
}

/**
 * toggle wifiscan percentages or quality icons
 * @since $dev
 * @access public
 * @param boolean enabled [false]
 */
void WiFiManager::setScanDispPerc(boolean enabled){
  _scanDispOptions = enabled;
}

/**
 * toggle configportal if autoconnect failed
 * if enabled, then the configportal will be activated on autoconnect failure
 * @since $dev
 * @access public
 * @param boolean enabled [true]
 */
void WiFiManager::setEnableConfigPortal(boolean enable)
{
    _enableConfigPortal = enable;
}

/**
 * toggle configportal if autoconnect failed
 * if enabled, then the configportal will be de-activated on wifi save
 * @since $dev
 * @access public
 * @param boolean enabled [true]
 */
void WiFiManager::setDisableConfigPortal(boolean enable)
{
    _disableConfigPortal = enable;
}

/**
 * set the hostname (dhcp client id)
 * @since $dev
 * @access public
 * @param  char* hostname 32 character hostname to use for sta+ap in esp32, sta in esp8266
 * @return bool false if hostname is not valid
 */
bool  WiFiManager::setHostname(const char * hostname){
  //@todo max length 32
  _hostname = std::string(hostname);
  return true;
}

bool  WiFiManager::setHostname(std::string hostname){
  //@todo max length 32
  _hostname = hostname;
  return true;
}

/**
 * set the soft ao channel, ignored if channelsync is true and connected
 * @param int32_t   wifi channel, 0 to disable
 */
void WiFiManager::setWiFiAPChannel(int32_t channel){
  _apChannel = channel;
}

/**
 * set the soft ap hidden
 * @param bool   wifi ap hidden, default is false
 */
void WiFiManager::setWiFiAPHidden(bool hidden){
  _apHidden = hidden;
}


/**
 * toggle showing erase wifi config button on info page
 * @param boolean enabled
 */
void WiFiManager::setShowInfoErase(boolean enabled){
  _showInfoErase = enabled;
}

/**
 * toggle showing update upload web ota button on info page
 * @param boolean enabled
 */
void WiFiManager::setShowInfoUpdate(boolean enabled){
  _showInfoUpdate = enabled;
}

/**
 * check if the config portal is running
 * @return bool true if active
 */
bool WiFiManager::getConfigPortalActive(){
  return configPortalActive;
}

/**
 * [getConfigPortalActive description]
 * @return bool true if active
 */
bool WiFiManager::getWebPortalActive(){
  return webPortalActive;
}


std::string WiFiManager::getWiFiHostname(){
  #ifdef ESP32
    return (std::string)WiFi.getHostname();
  #else
    return (std::string)WiFi.hostname();
  #endif
}

/**
 * [setTitle description]
 * @param std::string title, set app title
 */
void WiFiManager::setTitle(std::string title){
  _title = title;
}

/**
 * set menu items and order
 * if param is present in menu , params will be removed from wifi page automatically
 * eg.
 *  const char * menu[] = {"wifi","setup","sep","info","exit"};
 *  WiFiManager.setMenu(menu);
 * @since $dev
 * @param uint8_t menu[] array of menu ids
 */
void WiFiManager::setMenu(const char * menu[], uint8_t size){
#ifdef WM_DEBUG_LEVEL
  // DEBUG_WM(WM_DEBUG_DEV,"setmenu array");
  #endif
  _menuIds.clear();
  for(size_t i = 0; i < size; i++){
    for(size_t j = 0; j < _nummenutokens; j++){
      if((std::string)menu[i] == (__FlashStringHelper *)(_menutokens[j])){
        if((std::string)menu[i] == "param") _paramsInWifi = false; // param auto flag
        _menuIds.push_back(j);
      }
      delay(0);
    }
    delay(0);
  }
  #ifdef WM_DEBUG_LEVEL
  // DEBUG_WM(getMenuOut());
  #endif
}

/**
 * setMenu with vector
 * eg.
 * std::vector<const char *> menu = {"wifi","setup","sep","info","exit"};
 * WiFiManager.setMenu(menu);
 * tokens can be found in _menutokens array in strings_en.h
 * @shiftIncrement $dev
 * @param {[type]} std::vector<const char *>& menu [description]
 */
void WiFiManager::setMenu(std::vector<const char *>& menu){
#ifdef WM_DEBUG_LEVEL
  // DEBUG_WM(WM_DEBUG_DEV,"setmenu vector");
  #endif
  _menuIds.clear();
  for(auto menuitem : menu ){
    for(size_t j = 0; j < _nummenutokens; j++){
      if((std::string)menuitem == (__FlashStringHelper *)(_menutokens[j])){
        if((std::string)menuitem == "param") _paramsInWifi = false; // param auto flag
        _menuIds.push_back(j);
      }
    }
  }
  #ifdef WM_DEBUG_LEVEL
  // DEBUG_WM(WM_DEBUG_DEV,getMenuOut());
  #endif
}


/**
 * set params as sperate page not in wifi
 * NOT COMPATIBLE WITH setMenu! 
 * @todo scan menuids and insert param after wifi or something, same for ota
 * @param bool enable 
 * @since $dev
 */
void WiFiManager::setParamsPage(bool enable){
  _paramsInWifi  = !enable;
  setMenu(enable ? _menuIdsParams : _menuIdsDefault);
}

// GETTERS

/**
 * get config portal AP SSID
 * @since 0.0.1
 * @access public
 * @return std::string the configportal ap name
 */
std::string WiFiManager::getConfigPortalSSID() {
  return _apName;
}

/**
 * return the last known connection result
 * logged on autoconnect and wifisave, can be used to check why failed
 * get as readable std::string with getWLStatusString(getLastConxResult);
 * @since $dev
 * @access public
 * @return bool return wl_status codes
 */
uint8_t WiFiManager::getLastConxResult(){
  return _lastconxresult;
}

/**
 * check if wifi has a saved ap or not
 * @since $dev
 * @access public
 * @return bool true if a saved ap config exists
 */
bool WiFiManager::getWiFiIsSaved(){
  return WiFi_hasAutoConnect();
}

/**
 * getDefaultAPName
 * @since $dev
 * @return std::string 
 */
std::string WiFiManager::getDefaultAPName(){
  std::string hostString = std::string(WIFI_getChipId(),HEX);
  hostString.toUpperCase();
  // char hostString[16] = {0};
  // sprintf(hostString, "%06X", ESP.getChipId());  
  return _wifissidprefix + "_" + hostString;
}

/**
 * setCountry
 * @since $dev
 * @param std::string cc country code, must be defined in WiFiSetCountry, US, JP, CN
 */
void WiFiManager::setCountry(std::string cc){
  _wificountry = cc;
}

/**
 * setClass
 * @param std::string str body class std::string
 */
void WiFiManager::setClass(std::string str){
  _bodyClass = str;
}

/**
 * setDarkMode
 * @param bool enable, enable dark mode via invert class
 */
void WiFiManager::setDarkMode(bool enable){
  _bodyClass = enable ? "invert" : "";
}

/**
 * setHttpPort
 * @param uint16_t port webserver port number default 80
 */
void WiFiManager::setHttpPort(uint16_t port){
  _httpPort = port;
}


bool WiFiManager::preloadWiFi(std::string ssid, std::string pass){
  _defaultssid = ssid;
  _defaultpass = pass;
  return true;
}

// HELPERS

/**
 * getWiFiSSID
 * @since $dev
 * @param bool persistent
 * @return std::string
 */
std::string WiFiManager::getWiFiSSID(bool persistent){
  return WiFi_SSID(persistent);
}

/**
 * getWiFiPass
 * @since $dev
 * @param bool persistent
 * @return std::string
 */
std::string WiFiManager::getWiFiPass(bool persistent){
  return WiFi_psk(persistent);
} 

// DEBUG
// @todo fix DEBUG_WM(0,0);
template <typename Generic>
void WiFiManager::DEBUG_WM(Generic text) {
  DEBUG_WM(WM_DEBUG_NOTIFY,text,"");
}

template <typename Generic>
void WiFiManager::DEBUG_WM(wm_debuglevel_t level,Generic text) {
  if(_debugLevel >= level) DEBUG_WM(level,text,"");
}

template <typename Generic, typename Genericb>
void WiFiManager::DEBUG_WM(Generic text,Genericb textb) {
  DEBUG_WM(WM_DEBUG_NOTIFY,text,textb);
}

template <typename Generic, typename Genericb>
void WiFiManager::DEBUG_WM(wm_debuglevel_t level,Generic text,Genericb textb) {
  if(!_debug || _debugLevel < level) return;

  if(_debugLevel >= WM_DEBUG_MAX){
    #ifdef ESP8266
    // uint32_t free;
    // uint16_t max;
    // uint8_t frag;
    // ESP.getHeapStats(&free, &max, &frag);// @todo Does not exist in 2.3.0
    // _debugPort.printf("[MEM] free: %5d | max: %5d | frag: %3d%% \n", free, max, frag); 
    #elif defined ESP32
    // total_free_bytes;      ///<  Total free bytes in the heap. Equivalent to multi_free_heap_size().
    // total_allocated_bytes; ///<  Total bytes allocated to data in the heap.
    // largest_free_block;    ///<  Size of largest free block in the heap. This is the largest malloc-able size.
    // minimum_free_bytes;    ///<  Lifetime minimum free heap size. Equivalent to multi_minimum_free_heap_size().
    // allocated_blocks;      ///<  Number of (variable size) blocks allocated in the heap.
    // free_blocks;           ///<  Number of (variable size) free blocks in the heap.
    // total_blocks;          ///<  Total number of (variable size) blocks in the heap.
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    uint32_t free = info.total_free_bytes;
    uint16_t max  = info.largest_free_block;
    uint8_t frag = 100 - (max * 100) / free;
    _debugPort.printf("[MEM] free: %5lu | max: %5u | frag: %3u%% \n", free, max, frag);
    #endif
  }

  _debugPort.print(_debugPrefix);
  if(_debugLevel >= debugLvlShow) _debugPort.print("["+(std::string)level+"] ");
  _debugPort.print(text);
  if(textb){
    _debugPort.print(" ");
    _debugPort.print(textb);
  }
  _debugPort.println();
}

/**
 * [debugSoftAPConfig description]
 * @access public
 * @return {[type]} [description]
 */
void WiFiManager::debugSoftAPConfig(){
    
    #ifdef ESP8266
      softap_config config;
      wifi_softap_get_config(&config);
      #if !defined(WM_NOCOUNTRY)
        wifi_country_t country;
        wifi_get_country(&country);
      #endif
    #elif defined(ESP32)
      wifi_country_t country;
      wifi_config_t conf_config;
      esp_wifi_get_config(WIFI_IF_AP, &conf_config); // == ESP_OK
      wifi_ap_config_t config = conf_config.ap;
      esp_wifi_get_country(&country);
    #endif

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM("SoftAP Configuration");
    DEBUG_WM(FPSTR(D_HR));
    DEBUG_WM("ssid:            ",(char *) config.ssid);
    DEBUG_WM("password:        ",(char *) config.password);
    DEBUG_WM("ssid_len:        ",config.ssid_len);
    DEBUG_WM("channel:         ",config.channel);
    DEBUG_WM("authmode:        ",config.authmode);
    DEBUG_WM("ssid_hidden:     ",config.ssid_hidden);
    DEBUG_WM("max_connection:  ",config.max_connection);
    #endif
    #if !defined(WM_NOCOUNTRY) 
    #ifdef WM_DEBUG_LEVEL
      DEBUG_WM("country:         ",(std::string)country.cc);
      #endif
    DEBUG_WM("beacon_interval: ",(std::string)config.beacon_interval + "(ms)");
    DEBUG_WM(FPSTR(D_HR));
    #endif
}

/**
 * [debugPlatformInfo description]
 * @access public
 * @return {[type]} [description]
 */
void WiFiManager::debugPlatformInfo(){
  #ifdef ESP8266
    system_print_meminfo();
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM("[SYS] getCoreVersion():         ",ESP.getCoreVersion());
    DEBUG_WM("[SYS] system_get_sdk_version(): ",system_get_sdk_version());
    DEBUG_WM("[SYS] system_get_boot_version():",system_get_boot_version());
    DEBUG_WM("[SYS] getFreeHeap():            ",(std::string)ESP.getFreeHeap());
    #endif
  #elif defined(ESP32)
  #ifdef WM_DEBUG_LEVEL
    DEBUG_WM("[SYS] WM version: ", std::string((__FlashStringHelper *)WM_VERSION_STR) +" D:"+std::string(_debugLevel));
    DEBUG_WM("[SYS] Arduino version: ", VER_ARDUINO_STR);
    DEBUG_WM("[SYS] ESP SDK version: ", ESP.getSdkVersion());
    DEBUG_WM("[SYS] Free heap:       ", ESP.getFreeHeap());
    #endif

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM("[SYS] Chip ID:",WIFI_getChipId());
    DEBUG_WM("[SYS] Chip Model:", ESP.getChipModel());
    DEBUG_WM("[SYS] Chip Cores:", ESP.getChipCores());
    DEBUG_WM("[SYS] Chip Rev:",   ESP.getChipRevision());
    #endif
  #endif
}

int WiFiManager::getRSSIasQuality(int RSSI) {
  int quality = 0;

  if (RSSI <= -100) {
    quality = 0;
  } else if (RSSI >= -50) {
    quality = 100;
  } else {
    quality = 2 * (RSSI + 100);
  }
  return quality;
}

/** Is this an IP? */
boolean WiFiManager::isIp(std::string str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to std::string? */
std::string WiFiManager::toStringIp(IPAddress ip) {
  std::string res = "";
  for (int i = 0; i < 3; i++) {
    res += std::string((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += std::string(((ip >> 8 * 3)) & 0xFF);
  return res;
}

boolean WiFiManager::validApPassword(){
  // check that ap password is valid, return false
  if (_apPassword == NULL) _apPassword = "";
  if (_apPassword != "") {
    if (_apPassword.length() < 8 || _apPassword.length() > 63) {
    #ifdef WM_DEBUG_LEVEL
      DEBUG_WM("AccessPoint set password is INVALID or <8 chars");
      #endif
      _apPassword = "";
      return false; // @todo FATAL or fallback to empty , currently fatal, fail secure.
    }
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"AccessPoint set password is VALID");
    DEBUG_WM(WM_DEBUG_DEV,"ap pass",_apPassword);
    #endif
  }
  return true;
}

/**
 * encode htmlentities
 * @since $dev
 * @param  std::string str  str to replace entities
 * @return std::string      encoded str
 */
std::string WiFiManager::htmlEntities(std::string str, bool whitespace) {
  str.replace("&","&amp;");
  str.replace("<","&lt;");
  str.replace(">","&gt;");
  str.replace("'","&#39;");
  if(whitespace) str.replace(" ","&#160;");
  // str.replace("-","&ndash;");
  // str.replace("\"","&quot;");
  // str.replace("/": "&#x2F;");
  // str.replace("`": "&#x60;");
  // str.replace("=": "&#x3D;");
return str;
}

/**
 * [getWLStatusString description]
 * @access public
 * @param  {[type]} uint8_t status        [description]
 * @return {[type]}         [description]
 */
std::string WiFiManager::getWLStatusString(uint8_t status){
  if(status <= 7) return WIFI_STA_STATUS[status];
  return S_NA;
}

std::string WiFiManager::getWLStatusString(){
  uint8_t status = WiFi.status();
  if(status <= 7) return WIFI_STA_STATUS[status];
  return S_NA;
}

std::string WiFiManager::encryptionTypeStr(uint8_t authmode) {
#ifdef WM_DEBUG_LEVEL
  // DEBUG_WM("enc_tye: ",authmode);
  #endif
  return AUTH_MODE_NAMES[authmode];
}

std::string WiFiManager::getModeString(uint8_t mode){
  if(mode <= 3) return WIFI_MODES[mode];
  return S_NA;
}

bool WiFiManager::WiFiSetCountry(){
  if(_wificountry == "") return false; // skip not set

  #ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE,"WiFiSetCountry to",_wificountry);
  #endif

/*
  * @return
  *    - ESP_OK: succeed
  *    - ESP_ERR_WIFI_NOT_INIT: WiFi is not initialized by eps_wifi_init
  *    - ESP_ERR_WIFI_IF: invalid interface
  *    - ESP_ERR_WIFI_ARG: invalid argument
  *    - others: refer to error codes in esp_err.h
  */

  // @todo move these definitions, and out of cpp `esp_wifi_set_country(&WM_COUNTRY_US)`
  bool ret = true;
  // ret = esp_wifi_set_bandwidth(WIFI_IF_AP,WIFI_BW_HT20); // WIFI_BW_HT40
  #ifdef ESP32
  esp_err_t err = ESP_OK;
  // @todo check if wifi is init, no idea how, doesnt seem to be exposed atm ( check again it might be now! )
  if(WiFi.getMode() == WIFI_MODE_NULL){
      DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] cannot set country, wifi not init");        
  } // exception if wifi not init!
  // Assumes that _wificountry is set to one of the supported country codes : "01"(world safe mode) "AT","AU","BE","BG","BR",
  //               "CA","CH","CN","CY","CZ","DE","DK","EE","ES","FI","FR","GB","GR","HK","HR","HU",
  //               "IE","IN","IS","IT","JP","KR","LI","LT","LU","LV","MT","MX","NL","NO","NZ","PL","PT",
  //               "RO","SE","SI","SK","TW","US"
  // If an invalid country code is passed, ESP_ERR_WIFI_ARG will be returned
  // This also uses 802.11d mode, which matches the STA to the country code of the AP it connects to (meaning
  // that the country code will be overridden if connecting to a "foreign" AP)
  else {
    #ifndef WM_NOCOUNTRY
    err = esp_wifi_set_country_code(_wificountry.c_str(), true);
    #else
    DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] esp wifi set country is not available");
    err = true;
    #endif
  }
  #ifdef WM_DEBUG_LEVEL
    if(err){
      if(err == ESP_ERR_WIFI_NOT_INIT) DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] ESP_ERR_WIFI_NOT_INIT");
      else if(err == ESP_ERR_INVALID_ARG) DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] ESP_ERR_WIFI_ARG (invalid country code)");
      else if(err != ESP_OK)DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] unknown error",(std::string)err);
    }
  #endif
  ret = err == ESP_OK;
  
  #elif defined(ESP8266) && !defined(WM_NOCOUNTRY)
       // if(WiFi.getMode() == WIFI_OFF); // exception if wifi not init!
       if(_wificountry == "US") ret = wifi_set_country((wifi_country_t*)&WM_COUNTRY_US);
  else if(_wificountry == "JP") ret = wifi_set_country((wifi_country_t*)&WM_COUNTRY_JP);
  else if(_wificountry == "CN") ret = wifi_set_country((wifi_country_t*)&WM_COUNTRY_CN);
  #ifdef WM_DEBUG_LEVEL
  else DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] country code not found");
  #endif
  #endif
  
  #ifdef WM_DEBUG_LEVEL
  if(ret) DEBUG_WM(WM_DEBUG_VERBOSE,"[OK] esp_wifi_set_country: ",_wificountry);
  else DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] esp_wifi_set_country failed");  
  #endif
  return ret;
}

// set mode ignores WiFi.persistent 
bool WiFiManager::WiFi_Mode(WiFiMode_t m,bool persistent) {
    bool ret;
    #ifdef ESP8266
      if((wifi_get_opmode() == (uint8) m ) && !persistent) {
          return true;
      }
      ETS_UART_INTR_DISABLE();
      if(persistent) ret = wifi_set_opmode(m);
      else ret = wifi_set_opmode_current(m);
      ETS_UART_INTR_ENABLE();
    return ret;
    #elif defined(ESP32)
      if(persistent && esp32persistent) WiFi.persistent(true);
      ret = WiFi.mode(m); // @todo persistent check persistant mode, was eventually added to esp lib, but have to add version checking probably
      if(persistent && esp32persistent) WiFi.persistent(false);
      return ret;
    #endif
}
bool WiFiManager::WiFi_Mode(WiFiMode_t m) {
	return WiFi_Mode(m,false);
}

// sta disconnect without persistent
bool WiFiManager::WiFi_Disconnect() {
    #ifdef ESP8266
      if((WiFi.getMode() & WIFI_STA) != 0) {
          bool ret;
          #ifdef WM_DEBUG_LEVEL
          DEBUG_WM(WM_DEBUG_DEV,"WiFi station disconnect");
          #endif
          ETS_UART_INTR_DISABLE(); // @todo possibly not needed
          ret = wifi_station_disconnect();
          ETS_UART_INTR_ENABLE();        
          return ret;
      }
    #elif defined(ESP32)
    #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_DEV,"WiFi station disconnect");
      #endif
      return WiFi.disconnect(); // not persistent atm
    #endif
    return false;
}

// toggle STA without persistent
bool WiFiManager::WiFi_enableSTA(bool enable,bool persistent) {
#ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_DEV,"WiFi_enableSTA",(std::string) enable? "enable" : "disable");
    #endif
    #ifdef ESP8266
      WiFiMode_t newMode;
      WiFiMode_t currentMode = WiFi.getMode();
      bool isEnabled         = (currentMode & WIFI_STA) != 0;
      if(enable) newMode     = (WiFiMode_t)(currentMode | WIFI_STA);
      else newMode           = (WiFiMode_t)(currentMode & (~WIFI_STA));

      if((isEnabled != enable) || persistent) {
          if(enable) {
          #ifdef WM_DEBUG_LEVEL
          	if(persistent) DEBUG_WM(WM_DEBUG_DEV,"enableSTA PERSISTENT ON");
            #endif
              return WiFi_Mode(newMode,persistent);
          }
          else {
              return WiFi_Mode(newMode,persistent);
          }
      } else {
          return true;
      }
    #elif defined(ESP32)
      bool ret;
      if(persistent && esp32persistent) WiFi.persistent(true);
      ret =  WiFi.enableSTA(enable); // @todo handle persistent when it is implemented in platform
      if(persistent && esp32persistent) WiFi.persistent(false);
      return ret;
    #endif
}

bool WiFiManager::WiFi_enableSTA(bool enable) {
	return WiFi_enableSTA(enable,false);
}

bool WiFiManager::WiFi_eraseConfig() {
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_DEV,"WiFi_eraseConfig");
    #endif

    #ifdef ESP8266
      #ifndef WM_FIXERASECONFIG 
        return ESP.eraseConfig();
      #else
        // erase config BUG replacement
        // https://github.com/esp8266/Arduino/pull/3635
        const size_t cfgSize = 0x4000;
        size_t cfgAddr = ESP.getFlashChipSize() - cfgSize;

        for (size_t offset = 0; offset < cfgSize; offset += SPI_FLASH_SEC_SIZE) {
            if (!ESP.flashEraseSector((cfgAddr + offset) / SPI_FLASH_SEC_SIZE)) {
                return false;
            }
        }
        return true;
      #endif
    #elif defined(ESP32)

      bool ret;
      WiFi.mode(WIFI_AP_STA); // cannot erase if not in STA mode !
      WiFi.persistent(true);
      ret = WiFi.disconnect(true,true); // disconnect(bool wifioff, bool eraseap)
      delay(500);
      WiFi.persistent(false);
      return ret;
    #endif
}

uint8_t WiFiManager::WiFi_softap_num_stations(){
  #ifdef ESP8266
    return wifi_softap_get_station_num();
  #elif defined(ESP32)
    return WiFi.softAPgetStationNum();
  #endif
}

bool WiFiManager::WiFi_hasAutoConnect(){
  return WiFi_SSID(true) != "";
}

std::string WiFiManager::WiFi_SSID(bool persistent) const{

    #ifdef ESP8266
    struct station_config conf;
    if(persistent) wifi_station_get_config_default(&conf);
    else wifi_station_get_config(&conf);

    char tmp[33]; //ssid can be up to 32chars, => plus null term
    memcpy(tmp, conf.ssid, sizeof(conf.ssid));
    tmp[32] = 0; //nullterm in case of 32 char ssid
    return std::string(reinterpret_cast<char*>(tmp));
    
    #elif defined(ESP32)
    // bool res = WiFi.wifiLowLevelInit(true); // @todo fix for S3, not found
    // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if(persistent){
      wifi_config_t conf;
      esp_wifi_get_config(WIFI_IF_STA, &conf);
      return std::string(reinterpret_cast<const char*>(conf.sta.ssid));
    }
    else {
      if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
          return std::string();
      }
      wifi_ap_record_t info;
      if(!esp_wifi_sta_get_ap_info(&info)) {
          return std::string(reinterpret_cast<char*>(info.ssid));
      }
      return std::string();
    }
    #endif
}

std::string WiFiManager::WiFi_psk(bool persistent) const {
    #ifdef ESP8266
    struct station_config conf;

    if(persistent) wifi_station_get_config_default(&conf);
    else wifi_station_get_config(&conf);

    char tmp[65]; //psk is 64 bytes hex => plus null term
    memcpy(tmp, conf.password, sizeof(conf.password));
    tmp[64] = 0; //null term in case of 64 byte psk
    return std::string(reinterpret_cast<char*>(tmp));
    
    #elif defined(ESP32)
    // only if wifi is init
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
      return std::string();
    }
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    return std::string(reinterpret_cast<char*>(conf.sta.password));
    #endif
}

#ifdef ESP32
  #ifdef WM_ARDUINOEVENTS
  void WiFiManager::WiFiEvent(WiFiEvent_t event,arduino_event_info_t info){
  #else
  void WiFiManager::WiFiEvent(WiFiEvent_t event,system_event_info_t info){
    #define wifi_sta_disconnected disconnected
    #define ARDUINO_EVENT_WIFI_STA_DISCONNECTED SYSTEM_EVENT_STA_DISCONNECTED
    #define ARDUINO_EVENT_WIFI_SCAN_DONE SYSTEM_EVENT_SCAN_DONE
  #endif
    if(!_hasBegun){
      #ifdef WM_DEBUG_LEVEL
        // DEBUG_WM(WM_DEBUG_VERBOSE,"[ERROR] WiFiEvent, not ready");
      #endif
      // Serial.println("\n[EVENT] WiFiEvent logging (wm debug not available)");
      // Serial.print("[EVENT] ID: ");
      // Serial.println(event);
      return;
    }
    #ifdef WM_DEBUG_LEVEL
    // DEBUG_WM(WM_DEBUG_VERBOSE,"[EVENT]",event);
    #endif
    if(event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED){
    #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"[EVENT] WIFI_REASON: ",info.wifi_sta_disconnected.reason);
      #endif
      if(info.wifi_sta_disconnected.reason == WIFI_REASON_AUTH_EXPIRE || info.wifi_sta_disconnected.reason == WIFI_REASON_AUTH_FAIL){
        _lastconxresulttmp = 7; // hack in wrong password internally, sdk emit WIFI_REASON_AUTH_EXPIRE on some routers on auth_fail
      } else _lastconxresulttmp = WiFi.status();
      #ifdef WM_DEBUG_LEVEL
      if(info.wifi_sta_disconnected.reason == WIFI_REASON_NO_AP_FOUND) DEBUG_WM(WM_DEBUG_VERBOSE,"[EVENT] WIFI_REASON: NO_AP_FOUND");
      if(info.wifi_sta_disconnected.reason == WIFI_REASON_ASSOC_FAIL){
        if(_aggresiveReconn && _connectRetries<4) _connectRetries=4;
        DEBUG_WM(WM_DEBUG_VERBOSE,"[EVENT] WIFI_REASON: AUTH FAIL");
      }  
      #endif
      #ifdef esp32autoreconnect
      #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_VERBOSE,"[Event] SYSTEM_EVENT_STA_DISCONNECTED, reconnecting");
        #endif
        WiFi.reconnect();
      #endif
  }
  else if(event == ARDUINO_EVENT_WIFI_SCAN_DONE && _asyncScan){
    uint16_t scans = WiFi.scanComplete();
    WiFi_scanComplete(scans);
  }
}
#endif

void WiFiManager::WiFi_autoReconnect(){
  #ifdef ESP8266
    WiFi.setAutoReconnect(_wifiAutoReconnect);
  #elif defined(ESP32)
    // if(_wifiAutoReconnect){
      // @todo move to seperate method, used for event listener now
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"ESP32 event handler enabled");
      #endif
      using namespace std::placeholders;
      if(wm_event_id == 0) wm_event_id = WiFi.onEvent(std::bind(&WiFiManager::WiFiEvent,this,_1,_2));
    // }
  #endif
}

// Called when /update is requested
void WiFiManager::handleUpdate() {
  #ifdef WM_DEBUG_LEVEL
	DEBUG_WM(WM_DEBUG_VERBOSE,"<- Handle update");
  #endif
	if (captivePortal()) return; // If captive portal redirect instead of displaying the page
	std::string page = getHTTPHead(_title, C_update); // @token options
	std::string str = HTTP_ROOT_MAIN;
  str.replace(T_t, _title);
	str.replace(T_v, configPortalActive ? _apName : (getWiFiHostname() + " - " + WiFi.localIP().toString())); // use ip if ap is not active for heading
	page += str;

	page += HTTP_UPDATE;
	page += getHTTPEnd();

	HTTPSend(page);

}

// upload via /u POST
void WiFiManager::handleUpdating(){
  // @todo
  // cannot upload files in captive portal, file select is not allowed, show message with link or hide
  // cannot upload if softreset after upload, maybe check for hard reset at least for dev, ERROR[11]: Invalid bootstrapping state, reset ESP8266 before updating
  // add upload status to webpage somehow
  // abort upload if error detected ?
  // [x] supress cp timeout on upload, so it doesnt keep uploading?
  // add progress handler for debugging
  // combine route handlers into one callback and use argument or post checking instead of mutiple functions maybe, if POST process else server upload page?
  // [x] add upload checking, do we need too check file?
  // convert output to debugger if not moving to example
	
  // if (captivePortal()) return; // If captive portal redirect instead of displaying the page
  bool error = false;
  unsigned long _configPortalTimeoutSAV = _configPortalTimeout; // store cp timeout
  _configPortalTimeout = 0; // disable timeout

  // handler for the file upload, get's the sketch bytes, and writes
	// them through the Update object
	HTTPUpload& upload = server->upload();

  // UPLOAD START
	if (upload.status == UPLOAD_FILE_START) {
	  // if(_debug) Serial.setDebugOutput(true);
    uint32_t maxSketchSpace;
    
    // Use new callback for before OTA update
    if (_preotaupdatecallback != NULL) {
      _preotaupdatecallback();  // @CALLBACK
    }
    #ifdef ESP8266
    		WiFiUDP::stopAll();
    		maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    #elif defined(ESP32)
          // Think we do not need to stop WiFIUDP because we haven't started a listener
    		  // maxSketchSpace = (ESP.getFlashChipSize() - 0x1000) & 0xFFFFF000;
          // #define UPDATE_SIZE_UNKNOWN 0xFFFFFFFF // include update.h
          maxSketchSpace = UPDATE_SIZE_UNKNOWN;
    #endif

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE,"[OTA] Update file: ", upload.filename.c_str());
    #endif

    // Update.onProgress(THandlerFunction_Progress fn);
    // Update.onProgress([](unsigned int progress, unsigned int total) {
    //       Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    // });

  	if (!Update.begin(maxSketchSpace)) { // start with max available size
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] OTA Update ERROR", Update.getError());
        #endif
        error = true;
        Update.end(); // Not sure the best way to abort, I think client will keep sending..
  	}
	}
  // UPLOAD WRITE
  else if (upload.status == UPLOAD_FILE_WRITE) {
		// Serial.print(".");
		if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_ERROR,"[ERROR] OTA Update WRITE ERROR", Update.getError());
			//Update.printError(Serial); // write failure
      #endif
      error = true;
		}
	}
  // UPLOAD FILE END
  else if (upload.status == UPLOAD_FILE_END) {
		if (Update.end(true)) { // true to set the size to the current progress
      #ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_VERBOSE,"\n\n[OTA] OTA FILE END bytes: ", upload.totalSize);
			// Serial.printf("Updated: %u bytes\r\nRebooting...\r\n", upload.totalSize);
      #endif
		}
    else {
			// Update.printError(Serial);
      error = true;
		}
	}
  // UPLOAD ABORT
  else if (upload.status == UPLOAD_FILE_ABORTED) {
		Update.end();
		DEBUG_WM("[OTA] Update was aborted");
    error = true;
  }
  if(error) _configPortalTimeout = _configPortalTimeoutSAV;
	delay(0);
}

// upload and ota done, show status
void WiFiManager::handleUpdateDone() {
	DEBUG_WM(WM_DEBUG_VERBOSE, "<- Handle update done");
	// if (captivePortal()) return; // If captive portal redirect instead of displaying the page

	std::string page = getHTTPHead(S_options, C_update); // @token options
	std::string str  = HTTP_ROOT_MAIN;
  str.replace(T_t,_title);
	str.replace(T_v, configPortalActive ? _apName : WiFi.localIP().toString()); // use ip if ap is not active for heading
	page += str;

	if (Update.hasError()) {
		page += HTTP_UPDATE_FAIL;
    #ifdef ESP32
    page += "OTA Error: " + (std::string)Update.errorString();
    #else
    page += "OTA Error: " + (std::string)Update.getError();
    #endif
		DEBUG_WM("[OTA] update failed");
	}
	else {
		page += HTTP_UPDATE_SUCCESS;
		DEBUG_WM("[OTA] update ok");
	}
	page += getHTTPEnd();

	HTTPSend(page);

	delay(1000); // send page
	if (!Update.hasError()) {
		ESP.restart();
	}
}

#endif
