#ifndef _ESPNOW2MQTTCLIENT_HPP_
#define _ESPNOW2MQTTCLIENT_HPP_

#include <Arduino.h>
#include "criptMsg.hpp"
#include <pb_decode.h>
#include <pb_encode.h>
#include "messages.pb.h"

class EspNow2MqttClient
{
private:
    std::string name;
    std::string serverMac;
    CriptMsg crmsg = CriptMsg();
public:
    EspNow2MqttClient(std::string name, byte* key, std::string serverMac);
    ~EspNow2MqttClient();
    inline request_Operation createRequestOperationPing (int num);
    inline request_Operation createRequestOperationSend ( char* payload = "", char* queue = "out", bool retain = true);
    inline request_Operation createRequestOperationSubscribeQueue ( char* queue = "in", bool remove = true);
    //doSend()
    void doPing();
    //doSubscribe()
    void doRequests(request &rq);
    int pingCounter = 0;
private:
    int serialize (u8_t * buffer, request &rq);
};

EspNow2MqttClient::EspNow2MqttClient(std::string name, byte* key, std::string serverMac):
    name(name), 
    serverMac(serverMac)
{
    std::copy(crmsg.key, crmsg.key+crmsg.keySize, key);
}

EspNow2MqttClient::~EspNow2MqttClient()
{
}

inline request_Operation EspNow2MqttClient::createRequestOperationPing (int num){
    request_Operation result = request_Operation_init_zero;
    result.which_op = request_Operation_ping_tag;
    result.op.ping.num = num;
    return result;
}

inline request_Operation EspNow2MqttClient::createRequestOperationSend ( char* payload , char* queue , bool retain ){
    request_Operation result = request_Operation_init_zero;
    result.which_op = request_Operation_send_tag;
    strcpy(result.op.send.payload, payload);
    strcpy(result.op.send.queue, queue);
    result.op.send.persist = retain;
    return result;
}

inline request_Operation EspNow2MqttClient::createRequestOperationSubscribeQueue ( char* queue , bool remove ){
    request_Operation result = request_Operation_init_zero;
    result.which_op = request_Operation_qRequest_tag;
    strcpy(result.op.qRequest.queue, queue);
    result.op.qRequest.clear = remove;
    return result;
}

void EspNow2MqttClient::doPing(){
    request_Operation pingOp = createRequestOperationPing (pingCounter ++ );
    request requests = request_init_zero;
    strcpy (requests.client_id, this->name.c_str());
    requests.operations[0] = pingOp;
    requests.operations_count = 1;
    this->doRequests(requests);
}

void EspNow2MqttClient::doRequests(request &rq){
    uint8_t serialized [200];
    int serializedLength = this->serialize(serialized, rq);
    uint8_t ciphered [serializedLength];
    crmsg.encrypt(ciphered,serialized,serializedLength);
    //TODO: send ciphered
}

inline int EspNow2MqttClient::serialize (u8_t * buffer, request &rq){
    pb_ostream_t myStream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    auto encodeResult = pb_encode (&myStream, response_fields, &rq);
    int messageLength = myStream.bytes_written;
    return messageLength;
}


#endif // _ESPNOW2MQTTCLIENT_HPP_
