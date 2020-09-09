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
EspNow2MqttClient client = EspNow2MqttClient("tstCl", sharedKey, "c");

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
}

void loop() {
    testPing();
    delay(1000);
}