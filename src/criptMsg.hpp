#ifndef _criptMsg_hpp_
#define _criptMsg_hPP_

#include <Crypto.h>
#include <ChaCha.h>
#include <string.h>
#include <pgmspace.h>

class CriptMsg
{
private:
    ChaCha cc;
public:
    size_t keySize = 16;
    byte key[16] = {10,200,23,4,50,3,99,82,39,100,211,112,143,4,15,106};
    uint8_t rounds = 8;
    byte iv[8] = {10,20,30,40,50,60,70,80};
    byte counter[8] = {11,22,33,44,55,66,77,88};
    CriptMsg();
    ~CriptMsg();
    void decrypt(uint8_t *output, const uint8_t *input, size_t len){
        cc.setCounter(counter, sizeof(counter));
        cc.decrypt(output, input, len);
    }
    void encrypt(uint8_t *output, const uint8_t *input, size_t len){
        cc.setCounter(counter, sizeof(counter));
        cc.encrypt(output, input, len);
    }
    void encrypt(uint8_t *output, const char *input){
        this->encrypt(output, (uint8_t *)input, strlen((char * ) input) +1 );
    }
};

CriptMsg::CriptMsg()
{
        cc = ChaCha();
        cc.setKey(key, sizeof(key));
        cc.setIV(iv, sizeof(iv));
        cc.setCounter(counter, sizeof(counter));
        cc.setNumRounds(rounds);
}

CriptMsg::~CriptMsg()
{
}


#endif