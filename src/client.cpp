#include <Arduino.h>
#include "display.hpp"
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include "EspNow2MqttClient.hpp"
#include <esp_now.h>


Display display = Display();
byte sharedKey[16] = {10,200,23,4,50,3,99,82,39,100,211,112,143,4,15,106};
uint8_t gatewayMac[6] = {0xA4, 0xCF, 0x12, 0x25, 0x9A, 0x30};
EspNow2MqttClient client = EspNow2MqttClient("tstCl", sharedKey, gatewayMac, 0);

void onDataSentUpdateDisplay(const uint8_t *mac_addr, esp_now_send_status_t status) {
  display.print(3, status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail", true);
}

void displayMyMac(){
  char macStr[22];
  strcpy(macStr, "Mac ");
  strcat(macStr,WiFi.macAddress().c_str());
  display.print(8, macStr);
}

void testPing(){
  char pingNs[6];

  display.print(1, "ping", false);
  itoa(client.pingCounter, pingNs, 10);
  display.print(2, pingNs, true);
  client.doPing();
}

void setup() {
  display.init();
  displayMyMac();

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  client.onSendCB = onDataSentUpdateDisplay;
  int initcode;
  do {
    display.print(5,"             TRYING");
    initcode = client.init();
    switch (initcode)
    {
    case 1:
      display.print(5,"CANNOT INIT");
      break;
    case 2:
      display.print(5,"CANNOT PAIR");
      break;
    default:
      display.print(5,"PAIRED");
      break;
    }
    delay(1001);
  } while (initcode != 0);

}

void loop() {
    testPing();
    delay(2000);
}