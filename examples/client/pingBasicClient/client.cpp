#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

#include "EspNow2MqttClient.hpp"

byte sharedKey[16] = {10,200,23,4,50,3,99,82,39,100,211,112,143,4,15,106};
byte sharedChannel = 8 ;
uint8_t gatewayMac[6] = {0xA4, 0xCF, 0x12, 0x25, 0x9A, 0x30};
EspNow2MqttClient client = EspNow2MqttClient("pingCl", sharedKey, gatewayMac, sharedChannel);

/* optional function called when the ping comes back to client */
void onResponseArrivalPrintIt(response & rsp) {
  Serial.println("pong!");
}

/* optional function called when the data reaches the server (gateway) */
void onDataSentPrintIt(bool success) {
  Serial.println( success ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);

  client.onSentACK = onDataSentPrintIt;
  client.onReceiveSomething = onResponseArrivalPrintIt;

  int initcode;
  do {
    Serial.println("TRYING TO CONNECT");
    initcode = client.init();
    switch (initcode)
    {
    case 1:
      Serial.println("CANNOT INIT");
      break;
    case 2:
      Serial.println("CANNOT PAIR");
      break;
    default:
      Serial.println("PAIRED");
      break;
    }
    delay(1001);
  } while (initcode != 0);

}

void loop() {
    Serial.println("Ping sent");
    client.doPing();
    delay(5000);
}