#ifndef _ESPNOW2MQTTCLIENT_HPP_
#define _ESPNOW2MQTTCLIENT_HPP_

#include <Arduino.h>
#include "criptMsg.hpp"
#include <pb_decode.h>
#include <pb_encode.h>
#include "messages.pb.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <functional>
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

#define EN2MC_BUFFERSIZE 200

void onEspNowRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len);

// -- definition of class -------------------------------------------------------------------------
class EspNow2MqttClient
{
private:
    static EspNow2MqttClient* singletonInstance;
    std::string name;
    uint8_t serverMac[6] ;
    CriptMsg crmsg = CriptMsg();
    int channel;
public:
    EspNow2MqttClient(std::string name, byte* key, u8_t* serverMac, int channel = 0);
    ~EspNow2MqttClient();
    static EspNow2MqttClient* GetInstance() {return singletonInstance;}
    int init();
    int init(int channel);
    inline request createRequest (); 
    inline request_Operation createRequestOperationPing (int num);
    inline request_Operation createRequestOperationSend ( char* payload = "", char* queue = "out", bool retain = true);
    inline request_Operation createRequestOperationSubscribeQueue ( char* queue = "in", bool remove = true);
    bool doSend(char* payload = "", char* queue = "out", bool retain = true, int msgTpye = 0);
    void doPing(int msgTpye = 0);
    bool doSubscribe(char * queue, int msgTpye = 0);
    bool doRequests(request &rq);
    int pingCounter = 0;
private:
    int serialize (u8_t * buffer, request &rq);
    void deserialize (response &rsp, const u8_t * buffer, int lngt);
    esp_now_peer_info_t peerInfo;
    friend void onEspNowRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len);
public:
    std::function<void(bool success)> onSentACK = NULL;
    std::function<void(response &responses)> onReceiveSomething = NULL;
};

EspNow2MqttClient* EspNow2MqttClient::singletonInstance = nullptr;;

// -- callbacks of espNOW -------------------------------------------------------------------------
void onEspNowRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
    EspNow2MqttClient *instance = EspNow2MqttClient::GetInstance();
    if (instance && instance -> onReceiveSomething){
        response reponses = response_init_default;
        instance -> deserialize(reponses, incomingData, len);
        instance -> onReceiveSomething(reponses);
    }
}

void EspNow2Mqtt_onSentRecipe(const uint8_t *mac_addr, esp_now_send_status_t status) {
    EspNow2MqttClient *instance = EspNow2MqttClient::GetInstance();
    if (instance && instance->onSentACK){
        instance->onSentACK(status == ESP_OK);
    }
}

// -- class implementation ------------------------------------------------------------------------

EspNow2MqttClient::EspNow2MqttClient(std::string name, byte* key, u8_t* serverMac, int channel):
    name(name)
{
    std::copy(key, key+crmsg.keySize, crmsg.key);
    std::copy(serverMac, serverMac + 6, this->serverMac);
    this->channel = channel;
    this->singletonInstance = this;
}

EspNow2MqttClient::~EspNow2MqttClient()
{
}

int EspNow2MqttClient::init()
{
    //required wifi mode
    WiFi.mode(WIFI_AP_STA);

    //force wifi channel
    //WiFi.printDiag(Serial); 
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(this->channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
    //WiFi.printDiag(Serial); 

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return 1;
    }

    //register callbacks
    esp_now_register_recv_cb(onEspNowRecv);
    esp_now_register_send_cb(EspNow2Mqtt_onSentRecipe);

    // Register peer
    memcpy(peerInfo.peer_addr, serverMac, 6);
    peerInfo.channel = this->channel;  
    peerInfo.encrypt = false; //software chrypto
    
    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Failed to add peer");
        return 2;
    }

    return 0;
}

//to be used latter, when you did not know the espnow channel at the construction time
int EspNow2MqttClient::init(int channel)
{
    this->channel = channel;
    return init();
}

inline request_Operation EspNow2MqttClient::createRequestOperationPing (int num)
{
    request_Operation result = request_Operation_init_zero;
    result.which_op = request_Operation_ping_tag;
    result.op.ping.num = num;
    return result;
}

inline request EspNow2MqttClient::createRequest (  ) {
    request requests = request_init_zero;
    strcpy (requests.client_id, this->name.c_str());
    requests.operations_count = 0;
    return requests;
}

inline request_Operation EspNow2MqttClient::createRequestOperationSend ( char* payload , char* queue , bool retain )
{
    request_Operation result = request_Operation_init_zero;
    result.which_op = request_Operation_send_tag;
    strcpy(result.op.send.payload, payload);
    strcpy(result.op.send.queue, queue);
    result.op.send.persist = retain;
    return result;
}

inline request_Operation EspNow2MqttClient::createRequestOperationSubscribeQueue ( char* queue , bool remove )
{
    request_Operation result = request_Operation_init_zero;
    result.which_op = request_Operation_qRequest_tag;
    strcpy(result.op.qRequest.queue, queue);
    result.op.qRequest.clear = remove;
    return result;
}

void EspNow2MqttClient::doPing(int msgTpye)
{
    request requests = createRequest();
    request_Operation pingOp = createRequestOperationPing (pingCounter ++ );
    requests.operations[0] = pingOp;
    requests.operations_count=1;
    requests.message_type = msgTpye;
    this->doRequests(requests);
}


bool EspNow2MqttClient::doSend(char* payload, char* queue, bool retain, int msgTpye)
{
    request requests = createRequest();
    request_Operation sendOp = createRequestOperationSend(payload, queue, retain);
    requests.operations[0] = sendOp;
    requests.operations_count=1;
    requests.message_type = msgTpye;
    return this->doRequests(requests);
}

bool EspNow2MqttClient::doSubscribe(char * queue, int msgTpye)
{
    request requests = createRequest();
    request_Operation subscribeOp = createRequestOperationSubscribeQueue(queue,false);
    requests.operations[0] = subscribeOp;
    requests.operations_count=1;
    requests.message_type = msgTpye;
    return this->doRequests(requests);
}

bool EspNow2MqttClient::doRequests(request &rq)
{
    // serialize
    uint8_t serialized [EN2MC_BUFFERSIZE];
    int serializedLength = this->serialize(serialized, rq);
    // encrypt
    uint8_t ciphered [serializedLength];
    crmsg.encrypt(ciphered,serialized,serializedLength);
    // send ciphered
    esp_err_t result = esp_now_send(serverMac, ciphered, serializedLength);
    // result
    return result == ESP_OK;
}

inline int EspNow2MqttClient::serialize (u8_t * buffer, request &rq)
{
    pb_ostream_t myStream = pb_ostream_from_buffer(buffer, EN2MC_BUFFERSIZE);
    pb_encode (&myStream, request_fields, &rq);
    int messageLength = myStream.bytes_written;
    return messageLength;
}

inline void EspNow2MqttClient::deserialize (response &rsp, const u8_t * buffer, int lngt)
{
    uint8_t deciphered[lngt];
    crmsg.decrypt(deciphered,buffer,lngt);

    pb_istream_t iStream = pb_istream_from_buffer(deciphered, lngt); 
    pb_decode(&iStream, response_fields, &rsp);
}


#endif // _ESPNOW2MQTTCLIENT_HPP_
