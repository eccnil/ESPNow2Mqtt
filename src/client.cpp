#include <Arduino.h>
#include "display.hpp"
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <esp_now.h>
#include <esp_wifi.h>

long timeSent, timeSentRadio, timeack, timePreSetup, timePostSetup;
Display display = Display();

void displayTimeArriving()
{
  long arrivalTime = millis();
  char line[30];
  long lapseSent = timeack - timeSent;
  long lapse = arrivalTime - timeSent;
  long lapseRadio = arrivalTime - timeSentRadio;
  long lapseRadioSetup = timePostSetup - timePreSetup;

  snprintf(line, sizeof(line), "time %d+ %d, %d, %d", lapseRadioSetup, lapseSent, lapse, lapseRadio);
  display.print(4,line,true);
  Serial.println(line);
}

#include "EspNow2MqttClient.hpp"

byte sharedKey[16] = {10,200,23,4,50,3,99,82,39,100,211,112,143,4,15,106};
byte sharedChannel = 8 ;
uint8_t gatewayMac[6] = {0xA4, 0xCF, 0x12, 0x25, 0x9A, 0x30};
EspNow2MqttClient client = EspNow2MqttClient("tstCl", sharedKey, gatewayMac, sharedChannel);


void onDataSentUpdateDisplay(const uint8_t *mac_addr, esp_now_send_status_t status) {
  timeack = millis();
  display.print(3, status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail", false);
}

void displayMyMac()
{
  char macStr[22];
  strcpy(macStr, "Mac ");
  strcat(macStr,WiFi.macAddress().c_str());
  display.print(7, macStr);
}

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

void testPing()
{
  char pingNs[6];

  display.print(1, "ping", false);
  itoa(client.pingCounter, pingNs, 10);
  display.print(2, pingNs, false);
  client.doPing();
}

void testSend()
{
  display.print(1, "send", false);
  char pingNs[6];
  itoa(client.pingCounter, pingNs, 10);
  client.doSend("spnowq", pingNs, true);
}

void setup() {
  Serial.begin(115200);
  display.init();
  displayMyMac();


  client.onSendCB = onDataSentUpdateDisplay;
  int initcode;
  do {
    display.print(5,"             TRYING");
    timePreSetup = millis();
    initcode = client.init();
    timePostSetup = millis();
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
    timeSent = millis();
    testPing();
    //testSend();
    delay(2000);
}