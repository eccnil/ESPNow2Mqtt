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
    CriptMsg crmsg = CriptMsg();
    response gwResponse = response_init_zero;
    request decodedRequest = request_init_default;
    response emptyResponse = response_init_zero;
    request defaultRequest = request_init_default;
public:
    EspNow2MqttGateway(byte* key);
    ~EspNow2MqttGateway();
    int init();
    void espNowHandler(const uint8_t * mac_addr, const uint8_t *incomingData, int len);
private:
    void pingHandler(const uint8_t * mac_addr, request_Ping & ping, response_OpResponse & rsp);
    void sendHandler(const uint8_t * mac_addr, request_Send & ping, response_OpResponse & rsp);
    void subscribeHandler(const uint8_t * mac_addr, request_Subscribe & ping, response_OpResponse & rsp);
    void buildResponse (response_Result code, char * payload , response_OpResponse & rsp);
    void deserializeRequest(request &rq, const uint8_t *incomingData, int len);
public:
    std::function<void(request)> onReceiveCallback = NULL;
};

EspNow2MqttGateway::EspNow2MqttGateway(byte* key)
{
    std::copy(key, key+crmsg.keySize, crmsg.key);
}

EspNow2MqttGateway::~EspNow2MqttGateway()
{
}

int EspNow2MqttGateway::init()
{
    return 0;
}

void EspNow2MqttGateway::espNowHandler(const uint8_t * mac_addr, const uint8_t *incomingData, int len)
{
    //create request object
    decodedRequest = defaultRequest;
    deserializeRequest(decodedRequest,incomingData,len);
    
    //re-init response object
    gwResponse = emptyResponse;
    gwResponse.opResponses_count = decodedRequest.operations_count;

    //call handlers
    int count;
    for (count = 0; count < decodedRequest.operations_count; count++)
    {
        auto &op = decodedRequest.operations[count].op;
        auto &which_op = decodedRequest.operations[count].which_op;
        response_OpResponse &rsp = gwResponse.opResponses[count];

        switch (which_op)
        {
        case request_Operation_send_tag:
            sendHandler(mac_addr, op.send, rsp);
            break;
        case request_Operation_qRequest_tag:
            subscribeHandler(mac_addr, op.qRequest, rsp);
            break;
        case request_Operation_ping_tag:
            pingHandler(mac_addr, op.ping, rsp);
            break;
        default:
            break;
        }
    }
    //TODO: send back response
    //TODO: call callaback to debug
}

void EspNow2MqttGateway::pingHandler(const uint8_t * mac_addr, request_Ping & ping, response_OpResponse & rsp)
{
    buildResponse(response_Result_OK, NULL, rsp);
}
void EspNow2MqttGateway::sendHandler(const uint8_t * mac_addr, request_Send & send, response_OpResponse & rsp)
{
    //TODO: implement
    buildResponse(response_Result_NOK, "sin implementar", rsp);
}
void EspNow2MqttGateway::subscribeHandler(const uint8_t * mac_addr, request_Subscribe & subs, response_OpResponse & rsp)
{
    //TODO: implement
    buildResponse(response_Result_NOK, "sin implementar", rsp);
}

inline void EspNow2MqttGateway::buildResponse(response_Result code, char * payload , response_OpResponse & rsp)
{
    rsp.result_code = code;
    if(payload){
        strlcpy(rsp.payload, payload, sizeof(rsp.payload));
    }
}

void EspNow2MqttGateway::deserializeRequest(request &rq, const uint8_t *incomingData, int len)
{
    //decrypt
    uint8_t decripted[len];
    crmsg.decrypt(decripted, incomingData, len);

    //deserialize
    //decodedRequest = request_init_default;
    pb_istream_t iStream = pb_istream_from_buffer(decripted, len);
    pb_decode(&iStream, request_fields, &rq);
}


#endif // _ESPNOW2MQTTGATEWAY_HPP_
