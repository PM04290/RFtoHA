#pragma once


#define EEPROM_MAX_SIZE 256
#define EEPROM_TEXT_SIZE 50

#define JSON_MAX_SIZE 10000

char Wifi_ssid[EEPROM_TEXT_SIZE] = "";  // WiFi SSID
char Wifi_pass[EEPROM_TEXT_SIZE] = "";  // WiFi password

char mqtt_host[EEPROM_TEXT_SIZE] = "192.168.0.10";
int mqtt_port = 1883;
char mqtt_user[EEPROM_TEXT_SIZE] = "mqttuser";
char mqtt_pass[EEPROM_TEXT_SIZE] = "mqttpass";

char AP_ssid[9] = "lora2ha0";  // AP WiFi SSID
char AP_pass[9] = "12345678";  // AP WiFi password

#define DEBUG_SERIAL
#ifdef DEBUG_SERIAL
#define DEBUG(x) Serial.print(x)
#define DEBUGln(x) Serial.println(x)
#define DEBUGf(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG(x)
#define DEBUGln(x)
#define DEBUGf(x,y)
#endif 
