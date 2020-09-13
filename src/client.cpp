#include <Arduino.h>
#include "display.hpp"
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <esp_now.h>

long timeSent, timeSentRadio, timeack;
Display display = Display();

void displayTimeArriving()
{
  long arrivalTime = millis();
  char line[30];
  long lapseSent = timeack - timeSent;
  long lapse = arrivalTime - timeSent;
  long lapseRadio = arrivalTime - timeSentRadio;

  snprintf(line, sizeof(line), "time: %d, %d, %d",lapseSent, lapse, lapseRadio);
  display.print(4,line,true);
  Serial.println(line);
}

#include "EspNow2MqttClient.hpp"

byte sharedKey[16] = {10,200,23,4,50,3,99,82,39,100,211,112,143,4,15,106};
uint8_t gatewayMac[6] = {0xA4, 0xCF, 0x12, 0x25, 0x9A, 0x30};
EspNow2MqttClient client = EspNow2MqttClient("tstCl", sharedKey, gatewayMac, 0);


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

void testPing()
{
  char pingNs[6];

  display.print(1, "ping", false);
  itoa(client.pingCounter, pingNs, 10);
  display.print(2, pingNs, false);
  client.doPing();
}

void setup() {
  Serial.begin(115200);
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
    timeSent = millis();
    testPing();
    delay(2000);
}