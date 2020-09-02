
#include <Arduino.h>
#include "display.hpp"
#include "criptMsg.hpp"

#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

Display display = Display();
CriptMsg crmsg = CriptMsg();



void displayMyMac(){
  char macStr[22];
  strcpy(macStr, "Mac ");
  strcat(macStr,WiFi.macAddress().c_str());
  display.print(8, macStr);

  Serial.begin(115200);
  Serial.println("init chacha");
 
  char * plaintext =  "hola! Pepe";
  uint8_t result[200];
  int msglen= strlen((char * ) plaintext) +1 ;
  uint8_t ciphertext[msglen];

  ciphertext[msglen]=0; //ta mu mal pero vale
  Serial.println(msglen);
  Serial.println( plaintext);

  crmsg.encrypt(ciphertext, plaintext);

  Serial.println(sizeof(ciphertext));
  Serial.println((char *) ciphertext);


  crmsg.decrypt(result,ciphertext, msglen);

  char* out = (char*) result;
  Serial.println(strlen(out));
  Serial.println(out);

}

void setup() {
  display.init();
  displayMyMac();
}

void loop() {
    delay(1000);
}