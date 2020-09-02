#include <Arduino.h>
#include "display.hpp"
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

Display display = Display();

void displayMyMac(){
  char macStr[22];
  strcpy(macStr, "Mac ");
  strcat(macStr,WiFi.macAddress().c_str());
  display.print(8, macStr);
}

void setup() {
  display.init();
  displayMyMac();
}

void loop() {
    delay(1000);
}