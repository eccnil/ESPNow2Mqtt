#include <Arduino.h>
#include "display.hpp"
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include "EspNow2MqttClient.hpp"

Display display = Display();
byte sharedKey[16] = {10,200,23,4,50,3,99,82,39,100,211,112,143,4,15,106};
uint8_t gatewayMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
EspNow2MqttClient client = EspNow2MqttClient("tstCl", sharedKey, gatewayMac, 1);

void displayMyMac(){
  char macStr[22];
  strcpy(macStr, "Mac ");
  strcat(macStr,WiFi.macAddress().c_str());
  display.print(8, macStr);
}

void testPing(){
  char pingNs[6];

  display.print(1, "ping", false);
  display.print(2, pingNs);
  itoa(client.pingCounter, pingNs, 10);
  client.doPing();
}

void setup() {
  display.init();
  displayMyMac();

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  client.init();

}

void loop() {
    testPing();
    delay(1000);
}