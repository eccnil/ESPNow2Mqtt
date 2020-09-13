#if !defined(_ESPNOWUTIL_HPP_)
#define _ESPNOWUTIL_HPP_

#include <Arduino.h>
#include <esp_now.h>

#define EN2MC_BUFFERSIZE 200
class EspNowUtil 
{
private:
    esp_now_peer_info_t peerInfo;    
    int channel = 0;
public:    
    EspNowUtil(int channel = 0){ this->channel = channel; }
    ~EspNowUtil(){}
    void pair(const uint8_t * mac_addr);
    void send(const uint8_t * mac_addr, uint8_t * outputBuffer, int outputBufferLen);
};

void EspNowUtil::pair(const uint8_t * mac_addr) 
{
    bool existPeer = esp_now_is_peer_exist (mac_addr);
    if (!existPeer)
    {
        // Register peer
        memcpy(peerInfo.peer_addr, mac_addr, 6);
        peerInfo.channel = channel;  
        peerInfo.encrypt = false; //software chrypto
        
        // Add peer        
        if (esp_now_add_peer(&peerInfo) != ESP_OK){
            Serial.println("Failed to add peer");
        }
    }
}

inline void EspNowUtil::send(const uint8_t * mac_addr, uint8_t * outputBuffer, int outputBufferLen)
{
    pair(mac_addr);

    esp_err_t result = esp_now_send(mac_addr, outputBuffer, outputBufferLen);

    if (result != ESP_OK)
    {
        Serial.println("cannot send msg by espnow");
    }
}

#endif // _ESPNOWUTIL_HPP_
