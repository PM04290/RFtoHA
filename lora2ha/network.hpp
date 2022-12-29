#pragma once
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

#include "include_html.hpp"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

TimerHandle_t wifiReconnectTimer;

bool wifiOK1time = false;

String uniqueid = "?";
String version_major = "?";
String version_minor = "?";

boolean isValidNumber(String str) {
  for (byte i = 0; i < str.length(); i++)
  {
    if (!isDigit(str.charAt(i)))
    {
      return false;
    }
  }
  return str.length() > 0;
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void listDir(const char * dirname) {

  Serial.printf("Listing directory: %s\n", dirname);
  File root = SPIFFS.open(dirname);
  if (!root.isDirectory()) {
    Serial.println("No Dir");
  }
  File file = root.openNextFile();
  while (file) {
    Serial.print("  FILE: ");
    Serial.print(file.name());
    Serial.print("\tSIZE: ");
    Serial.println(file.size());
    file = root.openNextFile();
  }
}

void WiFiEvent(WiFiEvent_t event) {
  DEBUGf("[WiFi-event] %d\n", event);
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      DEBUGln("WiFi connected");
      DEBUG("IP address: ");
      DEBUGln(WiFi.localIP());
      wifiOK1time = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      DEBUGln("WiFi lost connection");
      //xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      if (wifiOK1time) {
        xTimerStart(wifiReconnectTimer, 0);
      }
      break;
  }
}

String getHTMLforDevice(uint8_t d, Device* dev)
{
  String blocD = html_device;
  String replaceGen = "";
  blocD.replace("#D#", String(d));
  if (dev != nullptr)
  {
    blocD.replace("%CNFADDRESS%", String(dev->getAddress()));
    blocD.replace("%CNFNAME%",  dev->getName());
    //
    for (int c = 0; c < dev->getNbChild(); c++)
    {
      replaceGen += "<div id='conf_child_" + String(d) + "_" + String(c) + "' class='flex-wrap'>chargement...</div>\n";
    }
  } else {
    blocD.replace("%CNFADDRESS%", "");
    blocD.replace("%CNFNAME%",  "");
    replaceGen += "<div id='conf_child_" + String(d) + "_0' class='flex-wrap'>chargement...</div>\n";
  }
  //
  blocD.replace("%GENCHILDS%", replaceGen);
  return blocD;
}

String getHTMLforChild(uint8_t d, uint8_t c, Child* ch)
{
  String blocC = html_child;
  String kcnf;
  blocC.replace("#D#", String(d));
  blocC.replace("#C#", String(c));
  if (ch != nullptr)
  {
    blocC.replace("%CNFC_ID%", String(ch->getId()));
    blocC.replace("%CNFC_LABEL%", String(ch->getLabel()));
    for (int n = 0; n <= 10; n++) {
      kcnf = "%CNFC_S" + String(n) + "%";
      blocC.replace(kcnf, (int)ch->getSensorType() == n ? "selected" : "");
    }
    for (int n = 0; n <= 5; n++) {
      kcnf = "%CNFC_D" + String(n) + "%";
      blocC.replace(kcnf, (int)ch->getDataType() == n ? "selected" : "");
    }
  } else
  {
    blocC.replace("%CNFC_ID%", "1");
    blocC.replace("%CNFC_LABEL%", "");
    for (int n = 0; n <= 10; n++) {
      kcnf = "%CNFC_S" + String(n) + "%";
      blocC.replace(kcnf, "");
    }
    for (int n = 0; n <= 5; n++) {
      kcnf = "%CNFC_D" + String(n) + "%";
      blocC.replace(kcnf, "");
    }
  }
  return blocC;
}

void sendConfigDevice()
{
  String blocD = "";
  for (int d = 0; d < hub.getNbDevice(); d++)
  {
    Device* dev = hub.getDevice(d);
    blocD += getHTMLforDevice(d, dev);
  }
  docJson.clear();
  docJson["cmd"] = "html";
  docJson["#conf_dev"] = blocD;
  String js;
  serializeJson(docJson, js);
  ws.textAll(js);
}

void sendConfigChild()
{
  for (uint8_t d = 0; d < hub.getNbDevice(); d++)
  {
    Device* dev = hub.getDevice(d);
    for (uint8_t c = 0; c < dev->getNbChild(); c++)
    {
      Child* ch = dev->getChild(c);
      if (ch != nullptr)
      {
        docJson.clear();
        docJson["cmd"] = "html";
        docJson["#conf_child_" + String(d) + "_" + String(c)] = getHTMLforChild(d, c, ch);
        String js;
        serializeJson(docJson, js);
        ws.textAll(js);
      }
    }
  }
}

void notifyConfig() {
  if (ws.availableForWriteAll()) {
    sendConfigDevice();
    sendConfigChild();
    ws.textAll("{\"cmd\":\"loadselect\"}");
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  DEBUGln("handleWebSocketMessage");
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    String js;
    data[len] = 0;
    DEBUGln((char*)data);
    String cmd = getValue((char*)data, ';', 0);
    String val = getValue((char*)data, ';', 1);
    if (cmd == "adddev")
    {
      // add blank device
      docJson.clear();
      docJson["cmd"] = "replace";
      docJson["#newdev_" + val] = getHTMLforDevice(val.toInt(), NULL);
      serializeJson(docJson, js);
      ws.textAll(js);

      // add blank child
      docJson.clear();
      docJson["cmd"] = "html";
      docJson["#conf_child_" + val + "_0"] = getHTMLforChild(val.toInt(), 0, NULL);
      js = "";
      serializeJson(docJson, js);
      ws.textAll(js);
    }
    if (cmd == "addchild")
    {
      String chd = getValue((char*)data, ';', 2);
      // add blank child
      docJson.clear();
      docJson["cmd"] = "html";
      docJson["#conf_child_" + val + "_" + chd] = getHTMLforChild(val.toInt(), chd.toInt(), NULL);
      js = "";
      serializeJson(docJson, js);
      ws.textAll(js);
    }
    ws.textAll("{\"cmd\":\"loadselect\"}");

    return;
  }
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      DEBUGf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      notifyConfig();
      break;
    case WS_EVT_DISCONNECT:
      DEBUGf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void onIndexRequest(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  File f = SPIFFS.open("/index.html", "r");
  if (f) {
    String html;
    while (f.available()) {
      html = f.readStringUntil('\n') + '\n';
      if (html.indexOf('%') > 0)
      {
        if (html.indexOf("%GENHTMLDEV%") > 0) {
          //
          String blocH = html_header;
          blocH.replace("%CNFUID%", uniqueid);
          blocH.replace("%CNFVMAJ%", version_major);
          blocH.replace("%CNFVMIN%", version_minor);
          html.replace("<!--%GENHTMLDEV%-->", blocH);
        } else
        {
          html.replace("%CNFCODE%", String(AP_ssid[7]));
          html.replace("%CNFMAC%", WiFi.macAddress());
          html.replace("%CNFSSID%", Wifi_ssid);
          html.replace("%CNFPWD%", Wifi_pass);
          html.replace("%CNFMQTTSRV%", mqtt_host);
          html.replace("%CNFMQTTUSER%", mqtt_user);
          html.replace("%CNFMQTTPASS%", mqtt_pass);
        }
        //
      }
      response->print(html);
    }
    f.close();
  }
  request->send(response);
}

void onConfigRequest(AsyncWebServerRequest * request)
{
  DEBUGln("web request");
  /*
  for (int i = 0; i < request->params(); i++)
  {
    AsyncWebParameter* p = request->getParam(i);
    DEBUGf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
  }
*/
  if (request->hasParam("cnf", true))
  {
    String cnf = request->getParam("cnf", true)->value();

    if (cnf == "conf")
    {
      DynamicJsonDocument docJSon(JSON_MAX_SIZE);
      JsonObject Jconfig = docJSon.to<JsonObject>();
      int params = request->params();
      for (int i = 0; i < params; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->isPost() && p->name() != "cnf") {
          //DEBUGf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
          String str = p->name();
          if (str == "uniqueid") {
            Jconfig["uniqueid"] = p->value();
          } else if (str == "version_major") {
            Jconfig["version_major"] = p->value();
          } else if (str == "version_minor") {
            Jconfig["version_minor"] = p->value();
          } else {
            // exemple : devices_0_name
            //      or : devices_1_childs_2_label
            String valdev = getValue(str, '_', 0);
            String keydev = getValue(str, '_', 1);
            String attrdev = getValue(str, '_', 2);
            String keychild = getValue(str, '_', 3);
            String attrchild = getValue(str, '_', 4);
            String sval(p->value().c_str());

            //DEBUGf("[%s][%s][%s][%s][%s] = %s\n", valdev, keydev, attrdev, keychild, attrchild, sval);
            if (keychild == "") {
              if (isValidNumber(sval))
              {
                Jconfig[valdev][keydev.toInt()][attrdev] = sval.toInt();
              } else
              {
                Jconfig[valdev][keydev.toInt()][attrdev] = sval;
              }
            } else
            {
              // on laisse le Label vide pour supprimer une ligne
              if (request->getParam(valdev + "_" + keydev + "_" + attrdev + "_"  + keychild + "_label", true)->value() != "") {
                if (isValidNumber(sval))
                {
                  Jconfig[valdev][keydev.toInt()][attrdev][keychild.toInt()][attrchild] = sval.toInt();
                } else
                {
                  Jconfig[valdev][keydev.toInt()][attrdev][keychild.toInt()][attrchild] = sval;
                }
              }
            }
          }
        }
      }
      // nettoyage des pages vides
      for (JsonArray::iterator it = Jconfig["devices"].as<JsonArray>().begin(); it != Jconfig["devices"].as<JsonArray>().end(); ++it) {
        if (!(*it).containsKey("childs")) {
          Jconfig["devices"].as<JsonArray>().remove(it);
        }
      }
      //
      Jconfig["end"] = true;
      String Jres;
      size_t Lres = serializeJson(docJSon, Jres);
      //DEBUGln(Jres);

      File file = SPIFFS.open("/config.json", "w");
      if (file) {
        file.write((byte*)Jres.c_str(), Lres);
        file.close();
        DEBUGln("Fichier de config enregistré");
      } else {
        DEBUGln("Erreur sauvegarde config");
      }
    }

    if (cnf == "wifi") {
      AP_ssid[7] = request->getParam("CODE", true)->value().c_str()[0];
      strcpy(Wifi_ssid, request->getParam("SSID", true)->value().c_str() );
      strcpy(Wifi_pass, request->getParam("PWD", true)->value().c_str() );
      strcpy(mqtt_host, request->getParam("MQTTSRV", true)->value().c_str() );
      strcpy(mqtt_user, request->getParam("MQTTUSER", true)->value().c_str() );
      strcpy(mqtt_pass, request->getParam("MQTTPASS", true)->value().c_str() );

      DEBUGln(AP_ssid);
      DEBUGln(Wifi_ssid);
      DEBUGln(Wifi_pass);
      DEBUGln(mqtt_host);
      DEBUGln(mqtt_user);
      DEBUGln(mqtt_pass);

      EEPROM.writeChar(0, AP_ssid[7]);
      EEPROM.writeString(1, Wifi_ssid);
      EEPROM.writeString(1 + (EEPROM_TEXT_SIZE * 1), Wifi_pass);
      EEPROM.writeString(1 + (EEPROM_TEXT_SIZE * 2), mqtt_host);
      EEPROM.writeString(1 + (EEPROM_TEXT_SIZE * 3), mqtt_user);
      EEPROM.writeString(1 + (EEPROM_TEXT_SIZE * 4), mqtt_pass);
      EEPROM.commit();
    }
  }
  request->send(200, "text/plain", "OK");
}

size_t content_len;
void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    DEBUGln("Update start");
    content_len = request->contentLength();
    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
      Update.printError(Serial);
    }
  }
  if (Update.write(data, len) != len) {
    Update.printError(Serial);
  }
  if (final) {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "20");
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true)) {
      Update.printError(Serial);
    } else {
      DEBUGln("Update complete");
      delay(100);
      yield();
      delay(100);

      ESP.restart();
    }
  }
}

void handleDoFile(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index)
  {
    if (filename.endsWith(".png"))
    {
      request->_tempFile = SPIFFS.open("/i/" + filename, "w");
    } else if (filename.endsWith(".css"))
    {
      request->_tempFile = SPIFFS.open("/css/" + filename, "w");
    } else if (filename.endsWith(".js"))
    {
      request->_tempFile = SPIFFS.open("/js/" + filename, "w");
    } else
    {
      request->_tempFile = SPIFFS.open("/" + filename, "w");
    }
  }
  if (len) {
    request->_tempFile.write(data, len);
  }
  if (final) {
    request->_tempFile.close();
    request->redirect("/");
  }
}

void updateProgress(size_t prg, size_t sz) {
  DEBUGf("Progress: %d%%\n", (prg * 100) / content_len);
}

void initWeb()
{
  server.serveStatic("/js", SPIFFS, "/js");
  server.serveStatic("/fonts", SPIFFS, "/fonts");
  server.serveStatic("/css", SPIFFS, "/css");
  server.serveStatic("/img", SPIFFS, "/img");
  server.serveStatic("/config.json", SPIFFS, "/config.json");
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/doconfig", HTTP_POST, onConfigRequest);
  server.on("/doupdate", HTTP_POST, [](AsyncWebServerRequest * request) {},
  [](AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final) {
    handleDoUpdate(request, filename, index, data, len, final);
  });
  server.on("/dofile", HTTP_POST, [](AsyncWebServerRequest * request) {
    request->send(200);
  },
  [](AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final) {
    handleDoFile(request, filename, index, data, len, final);
  });
  server.on("/restart", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "10");
    response->addHeader("Location", "/");
    request->send(response);
    delay(500);
    ESP.restart();
  });
  server.begin();
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  Update.onProgress(updateProgress);
  DEBUGln("HTTP server started");
}

void connectToWifi() {
  char txt[40];
  bool wifiok = false;

  WiFi.onEvent(WiFiEvent);

  DEBUG("MAC : ");
  DEBUGln(WiFi.macAddress());

  // Mode normal
  WiFi.begin(Wifi_ssid, Wifi_pass);
  int tentativeWiFi = 0;
  // Attente de la connexion au réseau WiFi / Wait for connection
  while ( WiFi.status() != WL_CONNECTED && tentativeWiFi < 20) {
    delay( 500 ); DEBUG( "." );
    tentativeWiFi++;
  }
  wifiok = WiFi.status() == WL_CONNECTED;

  if (wifiok == false) {
    DEBUGln ( F(" No wifi, set AP mode.") );
    // Mode AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_ssid, AP_pass);
    // Default IP Address is 192.168.4.1
    // if you want to change uncomment below
    // softAPConfig (local_ip, gateway, subnet)

    Serial.println();
    Serial.print("AP WIFI :"); Serial.println ( AP_ssid );
    Serial.print("AP IP Address: "); Serial.println(WiFi.softAPIP());
  }

  if (MDNS.begin(AP_ssid)) { // rf2mqtt0.local
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS ok");
  }
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
}
