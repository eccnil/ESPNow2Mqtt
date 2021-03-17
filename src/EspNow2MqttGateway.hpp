#if !defined(_ESPNOW2MQTTGATEWAY_HPP_)
#define _ESPNOW2MQTTGATEWAY_HPP_

#include <Arduino.h>
#include "criptMsg.hpp"
#include <PubSubClient.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include "messages.pb.h"
#include <esp_now.h>
#include "EspNowUtil.hpp"
#include <PubSubClient.h>
#include <map>
#include <set>
#include <functional>

#define MQTT_CLIENT_ID "EspNow"
#define MQTT_ROOT_TOPIC MQTT_CLIENT_ID "/"
#define MQTT_WILL_TOPIC MQTT_CLIENT_ID "_will"
#define MQTT_WILL_QUOS 1
#define MQTT_WILL_RETAIN false
#define MQTT_WILL_MSG "EspNow2MqttBridge died!"

void EspNow2Mqtt_onResponseSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void EspNow2Mqtt_onDataReceived(const uint8_t * mac_addr, const uint8_t *incomingData, int len);
void EspNow2Mqtt_mqttCallback(char* topic, byte* payload, unsigned int length);
// -- class definition ----------------------------------------------------------------------------
class EspNow2MqttGateway
{
private:
    static EspNow2MqttGateway *espNow2MqttGatewaySingleton;
    CriptMsg crmsg = CriptMsg();
    response gwResponse = response_init_zero;
    request decodedRequest = request_init_default;
    response emptyResponse = response_init_zero;
    request defaultRequest = request_init_default;
    EspNowUtil eNowUtil;
    PubSubClient mqttClient;
    char * mqttId;
    char * mqttUser;
    char * mqttPassword;
    std::map <String,String> topics;
    std::set <String> subscriptions;
public:
    EspNow2MqttGateway(byte* key, Client& cnx, const char * mqttServer, const int mtttport = 1883, int espnowChannel = 0, char* mqttID = MQTT_CLIENT_ID, char* mqttUser = NULL, char* mqttPassword = NULL);
    ~EspNow2MqttGateway();
    int init();
    static EspNow2MqttGateway* getSingleton() {return espNow2MqttGatewaySingleton;}
    void espNowHandler(const uint8_t * mac_addr, const uint8_t *incomingData, int len);
    void loop();
    int getNumberOfSubscriptions(){return this->subscriptions.size();}
    int getNumberOfMessages(){return this->topics.size();}
    void sendGwMqttMessage(char*topic, char*payload);
private:
    void mqttConnect();
    void resusbcribe();
    void pingHandler(const uint8_t * mac_addr, request_Ping & ping, response_OpResponse & rsp);
    void sendHandler(const uint8_t * mac_addr, char* clientId, request_Send & ping, response_OpResponse & rsp);
    void subscribeHandler(const uint8_t * mac_addr, char* clientId, request_Subscribe & ping, response_OpResponse & rsp);
    void buildResponse (response_Result code, const char * payload , response_OpResponse & rsp);
    String buildQueueName (char * clientId, char * name);
    bool deserializeRequest(request &rq, const uint8_t *incomingData, int len);
    int serializeResponse (uint8_t * buffer, response &rsp);
    friend void EspNow2Mqtt_onResponseSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    friend void EspNow2Mqtt_onDataReceived(const uint8_t * mac_addr, const uint8_t *incomingData, int len);
    friend void EspNow2Mqtt_mqttCallback(char* topic, byte* payload, unsigned int length);
public:
    std::function<void(bool ack, request&, response&)> onProcessedRequest = NULL;
    std::function<void(const uint8_t * mac_addr, const uint8_t *incomingData, int len)> onDataReceived = NULL;
    std::function<void(char* topic, uint8_t* payload, unsigned int length)> onMqttDataReceived = NULL;
};
EspNow2MqttGateway* EspNow2MqttGateway::espNow2MqttGatewaySingleton = nullptr;;

// -- friend functions ----------------------------------------------------------------------------
void EspNow2Mqtt_onDataReceived(const uint8_t * mac_addr, const uint8_t *incomingData, int len){
    EspNow2MqttGateway* instance = EspNow2MqttGateway::getSingleton();
    if (instance){
        if (instance->onDataReceived){
            instance->onDataReceived(mac_addr,incomingData, len);
        }
        instance->espNowHandler( mac_addr, incomingData, len);
    }
}

void EspNow2Mqtt_onResponseSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    EspNow2MqttGateway * instance = EspNow2MqttGateway::getSingleton();
    if(instance && instance->onProcessedRequest) {
        instance->onProcessedRequest(status == ESP_OK, instance->decodedRequest, instance->gwResponse);    
    }
}

void EspNow2Mqtt_mqttCallback(char* topic, uint8_t* payload, unsigned int length){
    EspNow2MqttGateway* instance = EspNow2MqttGateway::getSingleton();
    if(instance){
        if (payload){
            char charStr [length+1];
            memcpy(charStr,payload,length);
            charStr[length]=0;
            String strPayload(charStr);
            instance->topics.insert(std::pair<String,String>(String(topic), strPayload));
        }
        else{
            instance->topics.erase(String(topic));
        }
        if(instance->onMqttDataReceived){
            instance->onMqttDataReceived(topic, payload, length);
        }
    }

}

void EspNow2Mqtt_subscribe(){
    esp_now_register_recv_cb(EspNow2Mqtt_onDataReceived);
    esp_now_register_send_cb(EspNow2Mqtt_onResponseSent);
}

// -- class implementation ------------------------------------------------------------------------
EspNow2MqttGateway::EspNow2MqttGateway(byte* key, Client& cnx, const char * mqttServer, const int mtttPort, int espnowChannel, char* mqttID, char* mqttUser, char* mqttPassword):
eNowUtil(espnowChannel), 
mqttClient(mqttServer, mtttPort, EspNow2Mqtt_mqttCallback, cnx)
{
    std::copy(key, key+crmsg.keySize, crmsg.key);
    espNow2MqttGatewaySingleton = this;
    this->mqttUser = mqttUser; 
    this->mqttPassword = mqttPassword;
    this->mqttId = mqttID;
}

EspNow2MqttGateway::~EspNow2MqttGateway()
{
    mqttClient.disconnect();
}

int EspNow2MqttGateway::init()
{
    //connecto to mqtt
    mqttConnect();
    //init esp-now, gw will be registered as a handler for incoming messages
    Serial.println("initiating espnow");
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
    }
    return 0;
}

void EspNow2MqttGateway::espNowHandler(const uint8_t * mac_addr, const uint8_t *incomingData, int len)
{
    //create request object
    decodedRequest = defaultRequest;
    bool decoded = deserializeRequest(decodedRequest,incomingData,len);

    if (decoded){
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
                sendHandler(mac_addr, decodedRequest.client_id, op.send, rsp);
                break;
            case request_Operation_qRequest_tag:
                subscribeHandler(mac_addr, decodedRequest.client_id, op.qRequest, rsp);
                break;
            case request_Operation_ping_tag:
                pingHandler(mac_addr, op.ping, rsp);
                break;
            default:
                break;
            }
        }
        gwResponse.message_type = decodedRequest.message_type;
        //send back response
        uint8_t outputBuffer[EN2MC_BUFFERSIZE];
        int outputBufferLen = serializeResponse( outputBuffer, gwResponse );

        eNowUtil.send(mac_addr,outputBuffer, outputBufferLen);
    }
    else {
        Serial.println("error decoding payload");
    }
    
}

void EspNow2MqttGateway::pingHandler(const uint8_t * mac_addr, request_Ping & ping, response_OpResponse & rsp)
{
    buildResponse(response_Result_OK, NULL, rsp);
}

void EspNow2MqttGateway::sendHandler(const uint8_t * mac_addr, char* clientId, request_Send & send, response_OpResponse & rsp)
{
    if ( mqttClient.connected() ){
        String queue = buildQueueName(clientId, send.queue);
        bool sendStatus = mqttClient.publish(queue.c_str(), send.payload);
        if (sendStatus) {
            buildResponse(response_Result_OK, NULL, rsp);
        } else {
            buildResponse(response_Result_NOK, "cannot send", rsp);
        }
    } else {
        buildResponse(response_Result_NOMQTT, "no mqtt connection", rsp);
    }
}

void EspNow2MqttGateway::subscribeHandler(const uint8_t * mac_addr, char* clientId, request_Subscribe & subs, response_OpResponse & rsp)
{
    String queue = buildQueueName(clientId, subs.queue);
    auto subscriptionsIt = subscriptions.find(queue);
    if (subscriptions.find(queue) != subscriptions.end()){ //subscribed
        //subscribed & requesting data
        auto data = topics.find(queue);
        if (data != topics.end()){ //there are some data!
            buildResponse(response_Result_OK, (data->second).c_str(), rsp);
            topics.erase(data);
        } else {
            buildResponse(response_Result_NO_MSG, NULL, rsp);
        }
    } else { 
        if (mqttClient.subscribe(queue.c_str())){
            subscriptions.insert(queue);
            buildResponse(response_Result_NO_MSG, "now subscribed", rsp);
        } else {
            buildResponse(response_Result_NOK, "cannot subscribe", rsp);
        }
    }
}

void EspNow2MqttGateway::mqttConnect(){
    Serial.println("connecting to mqtt");
    bool mqttStatus = mqttClient.connect(mqttId, mqttUser, mqttPassword, MQTT_WILL_TOPIC, MQTT_WILL_QUOS, MQTT_WILL_RETAIN, MQTT_WILL_MSG);
    Serial.println(mqttStatus?"connected to mqtt":"cannot connect to mqtt");
}

void EspNow2MqttGateway::resusbcribe(){
    for (auto subscription : this->subscriptions){
        mqttClient.subscribe(subscription.c_str());
    }
    Serial.printf("resuscribed to %d topics\n", this->getNumberOfSubscriptions());
}

void EspNow2MqttGateway::loop (){
    if(mqttClient.connected()){
        mqttClient.loop();
    } else {
        mqttConnect();
        resusbcribe();
    }
}

String EspNow2MqttGateway::buildQueueName (char * clientId, char * name){
    String queue = String(MQTT_ROOT_TOPIC);
    queue.concat(clientId);
    queue.concat("/");
    queue.concat(name);
    return queue;
}


inline void EspNow2MqttGateway::buildResponse(response_Result code, const char * payload , response_OpResponse & rsp)
{
    rsp.result_code = code;
    if(payload){
        strlcpy(rsp.payload, payload, sizeof(rsp.payload));
    }
}

bool EspNow2MqttGateway::deserializeRequest(request &rq, const uint8_t *incomingData, int len)
{
    //decrypt
    uint8_t decripted[len];
    crmsg.decrypt(decripted, incomingData, len);

    //deserialize
    pb_istream_t iStream = pb_istream_from_buffer(decripted, len);
    return pb_decode(&iStream, request_fields, &rq);
}

inline int EspNow2MqttGateway::serializeResponse (uint8_t * buffer, response &rsp)
{
    //serialize
    uint8_t serializedBuffer[EN2MC_BUFFERSIZE];    
    pb_ostream_t myStream = pb_ostream_from_buffer(serializedBuffer, EN2MC_BUFFERSIZE);
    pb_encode (&myStream, response_fields, &rsp);
    int messageLength = myStream.bytes_written;

    //encrypt
    crmsg.encrypt(buffer,serializedBuffer,messageLength);

    return messageLength;
}

void EspNow2MqttGateway::sendGwMqttMessage(char*topic, char*payload)
{
    if ( mqttClient.connected() ){
        String queue = buildQueueName("gw", topic);
        bool sendStatus = mqttClient.publish(queue.c_str(), payload);
        if (!sendStatus) {
            Serial.printf ("Error cannot sendGwMqttMessage to %s topic {%s} because code %i", topic, payload, sendStatus);
        }
    } else {
        Serial.printf ("Error cannot sendGwMqttMessage to %s topic {%s} because not connected to mqtt", topic, payload);
    }
}

#endif // _ESPNOW2MQTTGATEWAY_HPP_
