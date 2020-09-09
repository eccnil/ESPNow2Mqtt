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


//EspNow2MqttGateway gw = EspNow2MqttGateway(0);
Display display = Display();

void onRq(request rq){
    display.print(1,rq.client_id);

    if(rq.operations_count>0)
    {
        if(rq.operations[0].which_op == request_Operation_ping_tag)
        {
            char pingN[6];
            itoa(rq.operations[0].op.ping.num, pingN, 10);
            display.print(3, pingN, false);
        }
    }

    char operationsCount[2];
    itoa(rq.operations_count, operationsCount, 10);
    display.print(2,operationsCount, true);
}

void displayMyMac(){
    char macStr[22];
    strcpy(macStr, "Mac ");
    strcat(macStr,WiFi.macAddress().c_str());
    display.print(8, macStr);
}

void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
    char macStr[18];
    Serial.print("Packet received from: ");
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.println(macStr);
    display.print(1,"Packet received from: ", false);
    display.print(2,macStr, true);
  
}

void setup() {
    display.init();
    displayMyMac();

    //Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA); 

        //Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        display.print(3,"Error initializing ESP-NOW");
        return;
    } else {
        display.print(3,"Ok initializing ESP-NOW");
    }

    // Once ESPNow is successfully Init, we will register for recv CB to
    // get recv packer info
    esp_now_register_recv_cb(OnDataRecv);


    //gw.init();
    //gw.onReceiveCallback = onRq;
}

void loop() {
    // put your main code here, to run repeatedly:
    delay(1000);
}