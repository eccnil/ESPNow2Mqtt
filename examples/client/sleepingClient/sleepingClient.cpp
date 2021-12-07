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
#include "secrets.h"


#define LED 2

byte sharedKey[16] = {10,200,23,4,50,3,99,82,39,100,211,112,143,4,15,106};
RTC_DATA_ATTR int sharedChannel = 0 ;
uint8_t gatewayMac[6] = {0xA4, 0xCF, 0x12, 0x25, 0x9A, 0x30};
EspNow2MqttClient client = EspNow2MqttClient("tstMltSlp", sharedKey, gatewayMac, sharedChannel);

//better as a global, to save stack space (very small at esp and poorly used by espnow)
request requests = client.createRequest(); 
const request_Operation doOffOperation = client.createRequestOperationSend("OFF", "cmd");
const request_Operation subscribeOperation = client.createRequestOperationSubscribeQueue("cmd");

bool weHaveResponse = false;
RTC_DATA_ATTR long timeEnd = 0;
RTC_DATA_ATTR int counter = 0;

// send multiple requests in a single package.
// much fastesr and easier to handle the responses
void testMultipleRequests()
{
  char lastMeasure[15];
  ltoa(timeEnd,lastMeasure,10);
  requests.operations[0] = client.createRequestOperationSend(lastMeasure, "time");
  requests.operations[1] = client.createRequestOperationPing(counter++);
  requests.operations[2] = subscribeOperation;
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
  requests.operations[0] = doOffOperation;
  requests.operations_count = 1;
  client.doRequests(requests);
}

//we only take care about the 3rd response, the queue subscription
//responses came in the same order of the requests
void processResponse( response & rsp)
{
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
      } else {
        weHaveResponse = true;
      }
    } else {
      weHaveResponse = true;
    }
  } else if (1 == rsp.opResponses_count ){ //cmd 'off' response
    weHaveResponse = true;
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

void gotoSleep(){
  
  timeEnd = millis();
  long timeMicros = 4000 * 1000L;
  esp_sleep_enable_timer_wakeup(timeMicros);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  weHaveResponse = false;

  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason){
    //case ESP_SLEEP_WAKEUP_EXT0 : break;
    //case ESP_SLEEP_WAKEUP_EXT1 : break; 
    case ESP_SLEEP_WAKEUP_TIMER : 
      break;
    //case ESP_SLEEP_WAKEUP_TOUCHPAD : break;
    //case ESP_SLEEP_WAKEUP_ULP : break;
    default:
      sharedChannel = getWiFiChannel(WIFI_SSID); 
      break;
  }

  int initcode;
  do {
    initcode = client.init(sharedChannel);
    if (initcode != 0) delay(50);
  } while (initcode != 0);

  client.onSentACK = onDataSentUpdateDisplay;
  client.onReceiveSomething = processResponse;
  testMultipleRequests ();
}

void loop() {
    if(weHaveResponse || millis() > 2000){
      gotoSleep();
    }
    delay(1);
}