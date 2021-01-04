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
#define DISPLAY_LINE_RESPONSE 4
#define DISPLAY_LINE__ 5
#define DISPLAY_LINE_ESPNOW_STATUS 6
#define DISPLAY_LINE_MAC 7

Display display = Display(); //epaper screen

byte sharedKey[16] = {10,200,23,4,50,3,99,82,39,100,211,112,143,4,15,106};
byte sharedChannel = 8 ;
uint8_t gatewayMac[6] = {0xA4, 0xCF, 0x12, 0x25, 0x9A, 0x30};
EspNow2MqttClient client = EspNow2MqttClient("tstMltSlp", sharedKey, gatewayMac, sharedChannel);

void displayDataOnCompletion( response & rsp)
{
  char line[30];

  if  (1 == rsp.opResponses_count)
  {
    int resultCode = rsp.opResponses[0].result_code;
    switch (resultCode)
    {
    case response_Result_OK:
      snprintf(line, sizeof(line), "ok: %s", rsp.opResponses[0].payload);
      break;
    case response_Result_NO_MSG:
      snprintf(line, sizeof(line), "ok but no message");
      break;
    default:
      snprintf(line, sizeof(line), "status: %d %s", resultCode, rsp.opResponses[0].payload);
      break;
    }
  } else {
    snprintf(line, sizeof(line), "error: %d", rsp.opResponses_count);
  }
  display.print(DISPLAY_LINE_RESPONSE,line,true);
  Serial.println(line);
}

void onDataSentUpdateDisplay(bool success) {
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

int counter = 0;

void testMultiple()
{
  request requests;
  display.print(DISPLAY_LINE_OPERATION, "multiple", true);
  requests = client.createRequest();
  requests.operations[0] = client.createRequestOperationSend("payload", "queue");
  requests.operations[1] = client.createRequestOperationPing(counter++);
  requests.operations[2] = client.createRequestOperationSubscribeQueue("cmd");
  requests.operations_count = 3;
  bool st = client.doRequests(requests);
  display.print(DISPLAY_LINE__, st?"sent":"error", true);
}

void setup() {
  Serial.begin(115200);
  display.init();
  displayMyMac();

  int initcode;
  do {
    display.print(DISPLAY_LINE_ESPNOW_STATUS, "             TRYING");
    initcode = client.init();
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
  client.onReceiveSomething = displayDataOnCompletion;
}

void loop() {
    testMultiple ();
    delay(3000);
    delay(3000);
}