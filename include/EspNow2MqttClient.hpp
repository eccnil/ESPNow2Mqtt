#ifndef _ESPNOW2MQTTCLIENT_HPP_
#define _ESPNOW2MQTTCLIENT_HPP_

#include <Arduino.h>
#include "criptMsg.hpp"
#include <pb_decode.h>
#include <pb_encode.h>
#include "messages.pb.h"
#include <esp_now.h>

//TODO: move
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

class EspNow2MqttClient
{
private:
    std::string name;
    uint8_t serverMac[6] ;
    CriptMsg crmsg = CriptMsg();
    int channel;
public:
    EspNow2MqttClient(std::string name, byte* key, u8_t* serverMac, int channel = 0);
    ~EspNow2MqttClient();
    void init();
    inline request_Operation createRequestOperationPing (int num);
    inline request_Operation createRequestOperationSend ( char* payload = "", char* queue = "out", bool retain = true);
    inline request_Operation createRequestOperationSubscribeQueue ( char* queue = "in", bool remove = true);
    //doSend()
    void doPing();
    //doSubscribe()
    bool doRequests(request &rq);
    int pingCounter = 0;
private:
    int serialize (u8_t * buffer, request &rq);
    void deserialize (response &rsp, u8_t * buffer, int lngt);
};

EspNow2MqttClient::EspNow2MqttClient(std::string name, byte* key, u8_t* serverMac, int channel):
    name(name)
{
    std::copy(key, key+crmsg.keySize, crmsg.key);
    std::copy(serverMac, serverMac + 6, this->serverMac);
    this->channel = channel;
}

EspNow2MqttClient::~EspNow2MqttClient()
{
}

void EspNow2MqttClient::init()
{
    esp_now_init();
    esp_now_register_send_cb(OnDataSent);

    // Register peer
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, serverMac, 6);
    peerInfo.channel = this->channel;  
    peerInfo.encrypt = false; //software chrypto
    
    // Add peer        
    esp_now_add_peer(&peerInfo);
}

inline request_Operation EspNow2MqttClient::createRequestOperationPing (int num)
{
    request_Operation result = request_Operation_init_zero;
    result.which_op = request_Operation_ping_tag;
    result.op.ping.num = num;
    return result;
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

void EspNow2MqttClient::doPing()
{
    request_Operation pingOp = createRequestOperationPing (pingCounter ++ );
    request requests = request_init_zero;
    strcpy (requests.client_id, this->name.c_str());
    requests.operations[0] = pingOp;
    requests.operations_count = 1;
    this->doRequests(requests);
}

bool EspNow2MqttClient::doRequests(request &rq)
{
    // serialize
    uint8_t serialized [200];
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
    pb_ostream_t myStream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    auto encodeResult = pb_encode (&myStream, response_fields, &rq);
    int messageLength = myStream.bytes_written;
    return messageLength;
}

inline void EspNow2MqttClient::deserialize (response &rsp, u8_t * buffer, int lngt)
{
    uint8_t deciphered[lngt];
    crmsg.decrypt(deciphered,buffer,lngt);

    pb_istream_t iStream = pb_istream_from_buffer(deciphered, lngt); 
    pb_decode(&iStream, response_fields, &rsp);
}


#endif // _ESPNOW2MQTTCLIENT_HPP_
