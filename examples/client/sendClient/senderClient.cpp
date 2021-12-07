/* example with LCD screen that sends single mesages*/

#include <Arduino.h>
#include "display.hpp"
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <esp_wifi.h>
#include "EspNow2MqttClient.hpp"

#define DISPLAY_LINE_OPERATION 1
#define DISPLAY_LINE_DATA_SENT 2
#define DISPLAY_LINE_DELIVERY_STATUS 3
#define DISPLAY_LINE_TIMING 4
#define DISPLAY_LINE_RESPONSE 5
#define DISPLAY_LINE_ESPNOW_STATUS 6
#define DISPLAY_LINE_MAC 7

long timeSent, timeack, timePreSetup, timePostSetup;
Display display = Display();

void displayTimeArriving( response & rsp)
{
  long arrivalTime = millis();
  char line[30];
  long lapseSent = timeack - timeSent;
  long lapse = arrivalTime - timeSent;
  long lapseRadioSetup = timePostSetup - timePreSetup;

  snprintf(line, sizeof(line), "time %d+ %d, %d", lapseRadioSetup, lapseSent, lapse);
  display.print(DISPLAY_LINE_TIMING,line,false);
  Serial.println(line);
  if  (1 == rsp.opResponses_count)
  {
    snprintf(line, sizeof(line), "status : %d", rsp.opResponses[0].result_code);
  } else {
    snprintf(line, sizeof(line), "error: %d", rsp.opResponses_count);
  }
  display.print(DISPLAY_LINE_RESPONSE,line,true);
  Serial.println(line);
}


byte sharedKey[16] = {10,200,23,4,50,3,99,82,39,100,211,112,143,4,15,106};
byte sharedChannel = 8 ;
uint8_t gatewayMac[6] = {0xA4, 0xCF, 0x12, 0x25, 0x9A, 0x30};
EspNow2MqttClient client = EspNow2MqttClient("tstCl", sharedKey, gatewayMac, sharedChannel);

int counter = 0;

void onDataSentUpdateDisplay(bool success) {
  timeack = millis();
  display.print(DISPLAY_LINE_DELIVERY_STATUS, success ? "Delivery Success" : "Delivery Fail", false);
}

void displayMyMac()
{
  char macStr[22];
  strcpy(macStr, "Mac ");
  strcat(macStr,WiFi.macAddress().c_str());
  display.print(DISPLAY_LINE_MAC, macStr);
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

  display.print(DISPLAY_LINE_OPERATION, "ping", false);
  itoa(client.pingCounter, pingNs, 10);
  display.print(DISPLAY_LINE_DATA_SENT, pingNs, false);
  client.doPing();
}

void testSend()
{
  display.print(DISPLAY_LINE_OPERATION, "send", false);
  char payload[6];
  itoa(counter, payload, 10);
  display.print(DISPLAY_LINE_DATA_SENT, payload, false);
  client.doSend(payload, "spnowq", true);
  counter ++;
}

void setup() {
  Serial.begin(115200);
  display.init();
  displayMyMac();

  int initcode;
  do {
    display.print(DISPLAY_LINE_ESPNOW_STATUS, "             TRYING");
    timePreSetup = millis();
    initcode = client.init();
    timePostSetup = millis();
    switch (initcode)
    {
    case 1:
      display.print(DISPLAY_LINE_ESPNOW_STATUS ,"CANNOT INIT");
      break;
    case 2:
      display.print(DISPLAY_LINE_ESPNOW_STATUS ,"CANNOT PAIR");
      break;
    default:
      display.print(DISPLAY_LINE_ESPNOW_STATUS ,"PAIRED");
      break;
    }
    delay(1001);
  } while (initcode != 0);

  client.onSentACK = onDataSentUpdateDisplay;
  client.onReceiveSomething = displayTimeArriving;
}

void loop() {
    timeSent = millis();
    //testPing();
    testSend();
    delay(2000);
}