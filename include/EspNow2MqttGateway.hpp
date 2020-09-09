#if !defined(_ESPNOW2MQTTGATEWAY_HPP_)
#define _ESPNOW2MQTTGATEWAY_HPP_

#include <Arduino.h>
#include "criptMsg.hpp"
#include <pb_decode.h>
#include <pb_encode.h>
#include "messages.pb.h"
#include <esp_now.h>

class EspNow2MqttGateway
{
private:
    int channel;
public:
    EspNow2MqttGateway(int channel);
    ~EspNow2MqttGateway();
    int init();
public:
    std::function<void(request)> onReceiveCallback = NULL;
};

EspNow2MqttGateway::EspNow2MqttGateway(int channel)
{
}

EspNow2MqttGateway::~EspNow2MqttGateway()
{
}

int EspNow2MqttGateway::init()
{
      //Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return 1;
    }
    return 0;
}


#endif // _ESPNOW2MQTTGATEWAY_HPP_
