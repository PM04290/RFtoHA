/*
  Designed for ESP32 Dev Kit
  Radio module : SX1278 (Ra-01)

  Wifi AP
    - default ssid : lora2ha
    - default password : 012345678
    - default address : 192.164.4.1 (lora2ha0.local)
*/
#include "config.h"
#include <EEPROM.h>
#include "FS.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <HAintegration.h>
#define RL_SX1278
#include <RadioLink.h>

//--- ESP32 SPI pin for LoRa ---
// SCK  : 18
// MISO : 19
// MOSI : 23
// NSS  : 5
// DIO0 : 2
// NRST 2 4

RadioLinkClass RLcomm;

StaticJsonDocument<JSON_MAX_SIZE> docJson;

#include "devices.hpp"
#include "network.hpp"

WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device, 12); // 12 trigger MAX

TimerHandle_t mqttReconnectTimer;

uint32_t curtime, oldtime = 0;
byte ntick = 0;

#define MAX_PACKET 10
rl_packet_t packetTable[MAX_PACKET];
byte idxReadTable = 0;
byte idxWriteTable = 0;

void onMqttMessage(const char* topic, const uint8_t* payload, uint16_t length) {
  char* pl = (char*)payload;
  pl[length] = 0;
}

void onMqttConnected()
{
  DEBUGln("Mqtt connected");
}

void onMqttDisconnected()
{
  DEBUGln("Mqtt disconnected");
}

bool getPacket(rl_packet_t* p)
{
  if (idxReadTable != idxWriteTable)
  {
    *p = packetTable[idxReadTable];
    //
    idxReadTable++;
    if (idxReadTable >= MAX_PACKET) {
      idxReadTable = 0;
    }
    return true;
  }
  return false;
}

void onReceive(rl_packet_t p)
{
  if (p.destinationID == 0)
  {
    packetTable[idxWriteTable++] = p;
    if (idxWriteTable >= MAX_PACKET)
    {
      idxWriteTable = 0;
    }
    if (idxWriteTable == idxReadTable)
    {
      idxReadTable++;
      if (idxReadTable >= MAX_PACKET)
      {
        idxReadTable = 0;
      }
    }
  }
}

void processDevice()
{
  rl_packet_t p;
  if (getPacket(&p))
  {
    //
    Child* ch = hub.getChildById(p.senderID, p.childID);
    if (ch != nullptr)
    {
      ch->doPacketForHA(p);
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (Serial.availableForWrite() == false) {
    delay(50);
  }
  DEBUGln("Start debug");

  if (!SPIFFS.begin()) {
    DEBUGln(F("SPIFFS Mount failed"));
  } else {
    DEBUGln(F("SPIFFS Mount succesfull"));
    listDir("/");
  }
  delay(50);

  if (!EEPROM.begin(EEPROM_MAX_SIZE))
  {
    DEBUGln("failed to initialise EEPROM");
  } else {
    char code = EEPROM.readChar(0);
    if (code >= '0' && code <= '9') {

      AP_ssid[7] = code;
      strcpy(Wifi_ssid, EEPROM.readString(1).c_str());
      strcpy(Wifi_pass, EEPROM.readString(1 + EEPROM_TEXT_SIZE * 1).c_str());
      strcpy(mqtt_host, EEPROM.readString(1 + EEPROM_TEXT_SIZE * 2).c_str());
      strcpy(mqtt_user, EEPROM.readString(1 + EEPROM_TEXT_SIZE * 3).c_str());
      strcpy(mqtt_pass, EEPROM.readString(1 + EEPROM_TEXT_SIZE * 4).c_str());

      DEBUGln(AP_ssid);
      DEBUGln(Wifi_ssid);
      DEBUGln(Wifi_pass);
      DEBUGln(mqtt_host);
      DEBUGln(mqtt_user);
      DEBUGln(mqtt_pass);
    }
  }
  if (RLcomm.begin(433E6, onReceive, NULL, 17))
  {
    DEBUGln("LORA OK");
  } else {
    DEBUGln("LORA ERROR");
  }
  connectToWifi();
  initWeb();

  if (loadConfig())
  {
    byte mac[6];
    WiFi.macAddress(mac);
    // HA
    device.setUniqueId(mac, sizeof(mac));
    device.setManufacturer("M&L");
    device.setModel("LoRa2HA");
    device.setName(AP_ssid);
    device.setSoftwareVersion("0.1");

    mqtt.setDataPrefix("lora2ha");
    //mqtt.setBufferSize(2000);
    mqtt.onMessage(onMqttMessage);
    mqtt.onConnected(onMqttConnected);
    mqtt.onDisconnected(onMqttDisconnected);
    if (mqtt.begin(mqtt_host, mqtt_user, mqtt_pass))
    {
      DEBUGln("mqtt ready");
    } else {
      DEBUGln("mqtt error");
    }
  }
}

void loop()
{
  curtime = millis();
  if (curtime > oldtime + 60000 || curtime < oldtime)
  {
    ntick++;

    if (ntick >= 30) {
      // TODO live beat
      ntick = 0;
    }
    oldtime = curtime;
  }
  mqtt.loop();
  processDevice();
}

bool loadConfig()
{
  DEBUGln("Loading config file");
  File file = SPIFFS.open("/config.json");
  if (!file || file.isDirectory())
  {
    DEBUGln("failed to open config file");
    return false;
  }
  Device* dev;

   DeserializationError error = deserializeJson(docJson, file);
  file.close();
  if (error) {
    DEBUGln("failed to deserialize config file");
    return false;
  }
  uniqueid = docJson["uniqueid"].as<String>();
  version_major = docJson["version_major"].as<String>();
  version_minor = docJson["version_minor"].as<String>();

  // walk device array
  JsonVariant devices = docJson["dev"];
  if (devices.is<JsonArray>())
  {
    for (JsonVariant deviceItem : devices.as<JsonArray>())
    {
      uint8_t address = deviceItem["address"].as<int>();
      const char* name = deviceItem["name"].as<const char*>();
      if (dev = hub.addDevice(new Device(address, name)))
      {
        // walk child array
        JsonVariant childs = deviceItem["childs"];
        if (childs.is<JsonArray>())
        {
          for (JsonVariant childItem : childs.as<JsonArray>())
          {
            uint8_t id = childItem["id"];
            const char* lbl = childItem["label"].as<const char*>();
            rl_device_t st = (rl_device_t)childItem["sensortype"].as<int>();
            rl_data_t dt = (rl_data_t)childItem["datatype"].as<int>();
            dev->addChild(new Child(dev->getName(), id, lbl, st, dt));
          }
        }
      }
    }
  }
  docJson.clear();
  DEBUGln("Config file loaded");
  return true;
}
