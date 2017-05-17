#include <ESP8266Wifi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

/*
  ===========================================
       Tanner Overly 2017
              github.com/TheAwkwardSquad
  ===========================================
*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#define resetPin 4 /* <-- comment out or change if you need GPIO 4 for other purposes */
//#define USE_DISPLAY /* <-- uncomment that if you want to use the display */

#ifdef USE_DISPLAY
  #include <Wire.h>
  
  //include the library you need
  #include "SSD1306.h"
  //#include "SH1106.h"
  
  //button pins
  #define upBtn D6
  #define downBtn D7
  #define selectBtn D5
  
  #define buttonDelay 180 //delay in ms
  
  //render settings
  #define fontSize 8
  #define rowsPerSite 8
  
  //create display(Adr, SDA-pin, SCL-pin)
  SSD1306 display(0x3c, D2, D1);
  //SH1106 display(0x3c, D2, D1);
  
  int rows = 3;
  int curRow = 0;
  int sites = 1;
  int curSite = 1;
  int lrow = 0;
#endif

String wifiMode = "";
String attackMode = "";
String scanMode = "SCAN";

bool warning = true;

extern "C" {
  #include "user_interface.h"
}

ESP8266WebServer server(80);

#include <EEPROM.h>
#include "data.h"
#include "NameList.h"
#include "APScan.h"
#include "ClientScan.h"
#include "Attack.h"
#include "Settings.h"
#include "SSIDList.h"

/* ========== DEBUG ========== */
const bool debug = true;
/* ========== DEBUG ========== */

NameList nameList;

APScan apScan;
ClientScan clientScan;
Attack attack;
Settings settings;
SSIDList ssidList;

void sniffer(uint8_t *buf, uint16_t len) {
  clientScan.packetSniffer(buf, len);
}

void startWifi() {
  Serial.println("\nStarting WiFi AP:");
  WiFi.mode(WIFI_STA);
  wifi_set_promiscuous_rx_cb(sniffer);
  WiFi.softAP((const char*)settings.ssid.c_str(), (const char*)settings.password.c_str(), settings.apChannel, settings.ssidHidden); //for an open network without a password change to:  WiFi.softAP(ssid);
  Serial.println("SSID     : '" + settings.ssid+"'");
  Serial.println("Password : '" + settings.password+"'");
  Serial.println("-----------------------------------------------");
  if (settings.password.length() < 8) Serial.println("WARNING: password must have at least 8 characters!");
  if (settings.ssid.length() < 1 || settings.ssid.length() > 32) Serial.println("WARNING: SSID length must be between 1 and 32 characters!");
  wifiMode = "ON";
}

void stopWifi() {
  Serial.println("stopping WiFi AP");
  Serial.println("-----------------------------------------------");
  WiFi.disconnect();
  wifi_set_opmode(STATION_MODE);
  wifiMode = "OFF";
}

void loadIndexHTML() {
  if(warning){
    sendFile(200, "text/html", data_indexHTML, sizeof(data_indexHTML));
  }else{
    sendFile(200, "text/html", data_apscanHTML, sizeof(data_apscanHTML));
  }
}
void loadAPScanHTML() {
  warning = false;
  sendFile(200, "text/html", data_apscanHTML, sizeof(data_apscanHTML));
}
void loadStationsHTML() {
  sendFile(200, "text/html", data_stationHTML, sizeof(data_stationHTML));
}
void loadAttackHTML() {
  sendFile(200, "text/html", data_attackHTML, sizeof(data_attackHTML));
}
void loadSettingsHTML() {
  sendFile(200, "text/html", data_settingsHTML, sizeof(data_settingsHTML));
}
void load404() {
  sendFile(200, "text/html", data_error404, sizeof(data_error404));
}
void loadInfoHTML(){
  sendFile(200, "text/html", data_infoHTML, sizeof(data_infoHTML));
}
//void loadLicense(){
//  sendFile(200, "text/plain", data_license, sizeof(data_license));
//}

void loadFunctionsJS() {
  sendFile(200, "text/javascript", data_functionsJS, sizeof(data_functionsJS));
}
void loadAPScanJS() {
  sendFile(200, "text/javascript", data_apscanJS, sizeof(data_apscanJS));
}
void loadStationsJS() {
  sendFile(200, "text/javascript", data_stationsJS, sizeof(data_stationsJS));
}
void loadAttackJS() {
  sendFile(200, "text/javascript", data_attackJS, sizeof(data_attackJS));
}
void loadSettingsJS() {
  sendFile(200, "text/javascript", data_settingsJS, sizeof(data_settingsJS));
}

void loadStyle() {
  sendFile(200, "text/css;charset=UTF-8", data_styleCSS, sizeof(data_styleCSS));
}


void startWiFi(bool start) {
  if (start) startWifi();
  else stopWifi();
  clientScan.clearList();
}

//==========AP-Scan==========
void startAPScan() {
  scanMode = "scanning...";
#ifdef USE_DISPLAY
  drawInterface();
#endif
  if (apScan.start()) {

#ifdef USE_DISPLAY
    apScan.sort();
    rows = 3;
    rows += apScan.results;
    sites = rows / rowsPerSite;
    if (rows % rowsPerSite > 0) sites++;
#endif

    server.send ( 200, "text/json", "true");
    attack.stopAll();
    scanMode = "SCAN";
  }
}

void sendAPResults() {
  apScan.sendResults();
}

void selectAP() {
  if (server.hasArg("num")) {
    apScan.select(server.arg("num").toInt());
    server.send( 200, "text/json", "true");
    attack.stopAll();
  }
}

//==========Client-Scan==========
void startClientScan() {
  if (server.hasArg("time") && apScan.getFirstTarget() > -1 && !clientScan.sniffing) {
    server.send(200, "text/json", "true");
    clientScan.start(server.arg("time").toInt());
    attack.stopAll();
  } else server.send( 200, "text/json", "Error: no selected access point");
}

void sendClientResults() {
  clientScan.send();
}
void sendClientScanTime() {
  server.send( 200, "text/json", (String)settings.clientScanTime );
}

void selectClient() {
  if (server.hasArg("num")) {
    clientScan.select(server.arg("num").toInt());
    attack.stop(0);
    server.send( 200, "text/json", "true");
  }
}

void addClientFromList(){
  if(server.hasArg("num")) {
    int _num = server.arg("num").toInt();
    clientScan.add(nameList.getMac(_num));
    
    server.send( 200, "text/json", "true");
  }else server.send( 200, "text/json", "false");
}

void setClientName() {
  if (server.hasArg("id") && server.hasArg("name")) {
    if(server.arg("name").length()>0){
      nameList.add(clientScan.getClientMac(server.arg("id").toInt()), server.arg("name"));
      server.send( 200, "text/json", "true");
    }
    else server.send( 200, "text/json", "false");
  }
}

void deleteName() {
  if (server.hasArg("num")) {
    int _num = server.arg("num").toInt();
    nameList.remove(_num);
    server.send( 200, "text/json", "true");
  }else server.send( 200, "text/json", "false");
}

void clearNameList() {
  nameList.clear();
  server.send( 200, "text/json", "true" );
}

void editClientName() {
  if (server.hasArg("id") && server.hasArg("name")) {
    nameList.edit(server.arg("id").toInt(), server.arg("name"));
    server.send( 200, "text/json", "true");
  }else server.send( 200, "text/json", "false");
}

void addClient(){
  if(server.hasArg("mac") && server.hasArg("name")){
    String macStr = server.arg("mac");
    macStr.replace(":","");
    Serial.println("add "+macStr+" - "+server.arg("name"));
    if(macStr.length() < 12 || macStr.length() > 12) server.send( 200, "text/json", "false");
    else{
      Mac _newClient;
      for(int i=0;i<6;i++){
        const char* val = macStr.substring(i*2,i*2+2).c_str();
        uint8_t valByte = strtoul(val, NULL, 16);
        Serial.print(valByte,HEX);
        Serial.print(":");
        _newClient.setAt(valByte,i);
      }
      Serial.println();
      nameList.add(_newClient,server.arg("name"));
      server.send( 200, "text/json", "true");
    }
  }
}

//==========Attack==========
void sendAttackInfo() {
  attack.sendResults();
}

void startAttack() {
  if (server.hasArg("num")) {
    int _attackNum = server.arg("num").toInt();
    if (apScan.getFirstTarget() > -1 || _attackNum == 1 || _attackNum == 2) {
      attack.start(server.arg("num").toInt());
      server.send ( 200, "text/json", "true");
    } else server.send( 200, "text/json", "false");
  }
}

void addSSID() {
  ssidList.add(server.arg("name"));
  server.send( 200, "text/json", "true");
}

void cloneSSID() {
  ssidList.addClone(server.arg("name"));
  server.send( 200, "text/json", "true");
}

void deleteSSID() {
  ssidList.remove(server.arg("num").toInt());
  server.send( 200, "text/json", "true");
}

void randomSSID() {
  ssidList._random();
  server.send( 200, "text/json", "true");
}

void clearSSID() {
  ssidList.clear();
  server.send( 200, "text/json", "true");
}

void resetSSID() {
  ssidList.load();
  server.send( 200, "text/json", "true");
}

void saveSSID() {
  ssidList.save();
  server.send( 200, "text/json", "true");
}

void restartESP() {
  server.send( 200, "text/json", "true");
  ESP.reset();
}

//==========Settings==========
void getSettings() {
  settings.send();
}

void saveSettings() {
  if (server.hasArg("ssid")) settings.ssid = server.arg("ssid");
  if (server.hasArg("ssidHidden")) {
    if (server.arg("ssidHidden") == "false") settings.ssidHidden = false;
    else settings.ssidHidden = true;
  }
  if (server.hasArg("password")) settings.password = server.arg("password");
  if (server.hasArg("apChannel")) {
    if (server.arg("apChannel").toInt() >= 1 && server.arg("apChannel").toInt() <= 14) {
      settings.apChannel = server.arg("apChannel").toInt();
    }
  }
  if (server.hasArg("ssidEnc")) {
    if (server.arg("ssidEnc") == "false") settings.attackEncrypted = false;
    else settings.attackEncrypted = true;
  }
  if (server.hasArg("scanTime")) settings.clientScanTime = server.arg("scanTime").toInt();
  if (server.hasArg("timeout")) settings.attackTimeout = server.arg("timeout").toInt();
  if (server.hasArg("deauthReason")) settings.deauthReason = server.arg("deauthReason").toInt();
  if (server.hasArg("packetRate")) settings.attackPacketRate = server.arg("packetRate").toInt();
  if (server.hasArg("apScanHidden")) {
    if (server.arg("apScanHidden") == "false") settings.apScanHidden = false;
    else settings.apScanHidden = true;
  }
  if (server.hasArg("useLed")) {
    if (server.arg("useLed") == "false") settings.useLed = false;
    else settings.useLed = true;
    attack.refreshLed();
  }
  if (server.hasArg("channelHop")) {
    if (server.arg("channelHop") == "false") settings.channelHop = false;
    else settings.channelHop = true;
  }
  if (server.hasArg("multiAPs")) {
    if (server.arg("multiAPs") == "false") settings.multiAPs = false;
    else settings.multiAPs = true;
  }

  settings.save();
  server.send( 200, "text/json", "true" );
}

void resetSettings() {
  settings.reset();
  server.send( 200, "text/json", "true" );
}

#ifdef USE_DISPLAY
void drawInterface() {
  display.clear();

  int _lrow = 0;
  for (int i = curSite * rowsPerSite - rowsPerSite; i < curSite * rowsPerSite; i++) {
    if (i == 0) display.drawString(3, i * fontSize, " -->  WiFi " + wifiMode);
    else if (i == 1) display.drawString(3, i * fontSize, " -->  " + scanMode);
    else if (i == 2) display.drawString(3, i * fontSize, " -->  " + attackMode + " attack");
    else if (i - 3 <= apScan.results) {
      display.drawString(3, _lrow * fontSize, apScan.getAPName(i - 3));
      if (apScan.getAPSelected(i - 3)) {
        display.drawVerticalLine(1, _lrow * fontSize, fontSize);
        display.drawVerticalLine(2, _lrow * fontSize, fontSize);
      }
    }
    if (_lrow == lrow) display.drawVerticalLine(0, _lrow * fontSize, fontSize);
    _lrow++;
  }

  display.display();
}
#endif

void setup() {

  Serial.begin(115200);
  if(debug){
    delay(2000);
    Serial.println("\nStarting...\n");
  }
  
#ifdef USE_DISPLAY
  display.init();
  display.setFont(Roboto_Mono_8);
  display.flipScreenVertically();
  drawInterface();
  pinMode(upBtn, INPUT_PULLUP);
  pinMode(downBtn, INPUT_PULLUP);
  pinMode(selectBtn, INPUT_PULLUP);
#endif

  attackMode = "START";
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  EEPROM.begin(4096);
  SPIFFS.begin();
  
  settings.load();
  if (debug) settings.info();
  nameList.load();
  ssidList.load();

#ifdef resetPin
  pinMode(resetPin, INPUT_PULLUP);
  if(digitalRead(resetPin) == LOW) settings.reset();
#endif

  startWifi();
  attack.stopAll();
  attack.generate();

  /* ========== Web Server ========== */

  /* HTML */
  server.onNotFound(load404);

  server.on("/", loadIndexHTML);
  server.on("/index.html", loadIndexHTML);
  server.on("/apscan.html", loadAPScanHTML);
  server.on("/stations.html", loadStationsHTML);
  server.on("/attack.html", loadAttackHTML);
  server.on("/settings.html", loadSettingsHTML);
  server.on("/info.html", loadInfoHTML);
  //server.on("/license", loadLicense);

  /* JS */
  server.on("/js/apscan.js", loadAPScanJS);
  server.on("/js/stations.js", loadStationsJS);
  server.on("/js/attack.js", loadAttackJS);
  server.on("/js/settings.js", loadSettingsJS);
  server.on("/js/functions.js", loadFunctionsJS);

  /* CSS */
  server.on ("/style.css", loadStyle);

  /* JSON */
  server.on("/APScanResults.json", sendAPResults);
  server.on("/APScan.json", startAPScan);
  server.on("/APSelect.json", selectAP);
  server.on("/ClientScan.json", startClientScan);
  server.on("/ClientScanResults.json", sendClientResults);
  server.on("/ClientScanTime.json", sendClientScanTime);
  server.on("/clientSelect.json", selectClient);
  server.on("/setName.json", setClientName);
  server.on("/addClientFromList.json", addClientFromList);
  server.on("/attackInfo.json", sendAttackInfo);
  server.on("/attackStart.json", startAttack);
  server.on("/settings.json", getSettings);
  server.on("/settingsSave.json", saveSettings);
  server.on("/settingsReset.json", resetSettings);
  server.on("/deleteName.json", deleteName);
  server.on("/clearNameList.json", clearNameList);
  server.on("/editNameList.json", editClientName);
  server.on("/addSSID.json", addSSID);
  server.on("/cloneSSID.json", cloneSSID);
  server.on("/deleteSSID.json", deleteSSID);
  server.on("/randomSSID.json", randomSSID);
  server.on("/clearSSID.json", clearSSID);
  server.on("/resetSSID.json", resetSSID);
  server.on("/saveSSID.json", saveSSID);
  server.on("/restartESP.json", restartESP);
  server.on("/addClient.json",addClient);

  server.begin();
}

void loop() {
  if (clientScan.sniffing) {
    if (clientScan.stop()) startWifi();
  } else {
    server.handleClient();
    attack.run();
  }

  if(Serial.available()){
    String input = Serial.readString();
    if(input == "reset" || input == "reset\n" || input == "reset\r" || input == "reset\r\n"){
      settings.reset();
    }
  }

#ifdef USE_DISPLAY


  //go up
  if (digitalRead(upBtn) == LOW && curRow > 0) {
    curRow--;
    if (lrow - 1 < 0) {
      lrow = rowsPerSite - 1;
      curSite--;
    } else lrow--;
    delay(buttonDelay);

  // ===== go down ===== 
  } else if (digitalRead(downBtn) == LOW && curRow < rows - 1) {
    curRow++;
    if (lrow + 1 >= rowsPerSite) {
      lrow = 0;
      curSite++;
    } else lrow++;
    delay(buttonDelay);
    
  // ===== select ===== 
  } else if (digitalRead(selectBtn) == LOW) {
    //WiFi on/off
    if (curRow == 0) {
      if (wifiMode == "ON") stopWifi();
      else startWifi();
    
    // ===== scan for APs ===== 
    } else if (curRow == 1) {
      startAPScan();
      drawInterface();

    // ===== start,stop attack ===== 
    } else if (curRow == 2) {
      if (attackMode == "START" && apScan.getFirstTarget() > -1) attack.start(0);
      else if (attackMode == "STOP") attack.stop(0);
    } 
    
    // ===== select APs ===== 
    else if (curRow >= 3) {
      attack.stop(0);
      apScan.select(curRow - 3);
    }
    delay(buttonDelay);
  }
  drawInterface();
#endif

}

