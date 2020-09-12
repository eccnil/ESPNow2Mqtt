#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include "display.hpp"
#include "EspNow2MqttGateway.hpp"
#include "messages.pb.h"
#include <esp_now.h>


Display display = Display();
byte sharedKey[16] = {10,200,23,4,50,3,99,82,39,100,211,112,143,4,15,106};
EspNow2MqttGateway gw = EspNow2MqttGateway(sharedKey);


void onPostRq(request &rq, response &rsp ){
    char line[13];
    for (char opCount = 0; opCount < rq.operations_count; opCount ++)
    {
        int lineNum = 3 + opCount;
        switch (rq.operations[opCount].which_op)
        {
        case request_Operation_ping_tag:
            snprintf(line, sizeof(line), "ping: %d", rq.operations[opCount].op.ping.num );
            break;
        
        default:
            snprintf(line, sizeof(line), "unknown op");
            break;
        }
        display.print(lineNum,line,false);
    }
    snprintf(line, sizeof(line), "%s: %d ops",
        rq.client_id, 
        rq.operations_count);
    display.print(2,line,true);
}

void displayMyMac(){
    char macStr[22];
    strcpy(macStr, "Mac ");
    strcat(macStr,WiFi.macAddress().c_str());
    display.print(8, macStr);
}

void onEspNowRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
    char macStr[18+1+4]; //18 mac + 1 space + 3 len
    Serial.print("Packet received from: ");
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x %db",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], len);
    Serial.println(macStr);
    display.print(2,"---", false);
    display.print(1,macStr, true);

    gw.espNowHandler( mac_addr, incomingData, len);
  
}

void setup() {
    Serial.begin(115200);
    display.init();
    displayMyMac();

    //Set device as a Wi-Fi Station
    //TODO: remove for mqtt connection
    WiFi.mode(WIFI_STA); 

    //init gateway
    gw.init();
    gw.onReceivePostCallback = onPostRq;

    //init esp-now, gw will be registered as a handler for incoming messages
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
    }
    esp_now_register_recv_cb(onEspNowRecv);
}

void loop() {
    // put your main code here, to run repeatedly:
    delay(1000);
}