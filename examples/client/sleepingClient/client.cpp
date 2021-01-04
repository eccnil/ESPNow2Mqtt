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


#define LED 2

byte sharedKey[16] = {10,200,23,4,50,3,99,82,39,100,211,112,143,4,15,106};
byte sharedChannel = 8 ;
uint8_t gatewayMac[6] = {0xA4, 0xCF, 0x12, 0x25, 0x9A, 0x30};
EspNow2MqttClient client = EspNow2MqttClient("tstMltSlp", sharedKey, gatewayMac, sharedChannel);

int counter = 0;
// send multiple requests in a single package.
// much fastesr and easier to handle the responses
void testMultipleRequests()
{
  request requests = client.createRequest();
  requests.operations[0] = client.createRequestOperationSend("payload", "queue");
  requests.operations[1] = client.createRequestOperationPing(counter++);
  requests.operations[2] = client.createRequestOperationSubscribeQueue("cmd");
  requests.operations_count = 3;
  bool st = client.doRequests(requests);
}


//simulates doing something, like activateing a releay or something
//actually blinks the onboard led during a sencod
void do_ON(){
    digitalWrite(LED, HIGH);
    delay(1001);
    digitalWrite(LED, LOW);
}

//telling mqtt the "thing" is off again
void do_OFF()
{
  request requests = client.createRequest();
  requests.operations[0] = client.createRequestOperationSend("OFF", "cmd");
  requests.operations_count = 1;
  bool st = client.doRequests(requests);
}

//we only take care about the 3rd response, the queue subscription
//responses came in the same order of the requests
void processResponse( response & rsp)
{
  Serial.print("response ");Serial.println(rsp.opResponses_count);
  if  (3 == rsp.opResponses_count){
    int cmdResultCode = rsp.opResponses[2].result_code;
    Serial.print("response code ");Serial.println(cmdResultCode);
    if (cmdResultCode == response_Result_OK){
      String command (rsp.opResponses[2].payload);
      Serial.print("response payload ");Serial.println(command.c_str());
      if(command.equalsIgnoreCase("ON")){
        Serial.print("response doing on ");
        do_ON();
        Serial.print("response doing off again ");
        do_OFF();
      }
    }
  }
}

//doing nothing
void onDataSentUpdateDisplay(bool success) {
  //display.print(DISPLAY_LINE_DELIVERY_STATUS, success ? "Delivery Success" : "Delivery Fail", false);
}

//we can init our channel number from wifi ssid, but it consts seconds so only in case of restarting
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

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  int initcode;
  do {
    initcode = client.init();
    delay(500);
  } while (initcode != 0);

  client.onSentACK = onDataSentUpdateDisplay;
  client.onReceiveSomething = processResponse;
}

void loop() {
    testMultipleRequests ();
    delay(3000);
    delay(3000);
}