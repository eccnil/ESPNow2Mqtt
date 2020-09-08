
#include <Arduino.h>
#include "display.hpp"
#include "criptMsg.hpp"
#include <pb_decode.h>
#include <pb_encode.h>
#include "messages.pb.h"

#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

Display display = Display();
CriptMsg crmsg = CriptMsg();



void displayMyMac(){
  //display mac
  char macStr[22];
  strcpy(macStr, "Mac ");
  strcat(macStr,WiFi.macAddress().c_str());
  display.print(8, macStr);

}

void testChacha (){
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

void testProtobuffResp(){
  int messageLength;
  response_OpResponse res1 =response_OpResponse_init_zero;
  res1.result_code = response_Result_NOMQTT;
  strcpy(res1.payload, "bla bla");
  response myresponse = response_init_zero;
  myresponse.opResponses[0]=res1;
  myresponse.opResponses_count = 1;

  uint8_t buffer[200];
  pb_ostream_t myStream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  auto encodeResult = pb_encode (&myStream, response_fields, &myresponse);
  messageLength = myStream.bytes_written;

  Serial.println("bla bla");
  Serial.println(encodeResult);
  Serial.println(messageLength);
  Serial.println((char*) buffer );

  response decodedResponse = response_init_default;
  pb_istream_t iStream = pb_istream_from_buffer(buffer, messageLength);
  pb_decode(&iStream, response_fields, &decodedResponse);

  Serial.println(decodedResponse.opResponses[0].payload);
  Serial.println(decodedResponse.opResponses[0].result_code);


}

void testProtobuffRq(){
  int messageLength;

  request_Ping ping1 = request_Ping_init_zero;
  ping1.num=66;
  request_Operation op1 = request_Operation_init_zero;
  op1.op.ping = ping1;
  op1.which_op = request_Operation_ping_tag;
  request_Send send2 = request_Send_init_zero;
  strcpy(send2.queue,"sendq");
  strcpy(send2.payload,"{'mytest':0}");
  send2.persist=true;
  request_Operation op2 = request_Operation_init_zero;
  op2.op.send = send2;
  op2.which_op = request_Operation_send_tag;
  request_Subscribe subs3 = request_Subscribe_init_zero;
  strcpy(subs3.queue,"subsq");
  subs3.clear=false;
  request_Operation op3 = request_Operation_init_zero;
  op3.op.qRequest = subs3;
  op3.which_op = request_Operation_qRequest_tag;
  request myRq = request_init_zero;
  strcpy(myRq.client_id,"test");
  myRq.operations[0]=op1;
  myRq.operations[1]=op2;
  myRq.operations[2]=op3;
  myRq.operations_count=3;

  uint8_t buffer[200];
  pb_ostream_t myStream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  auto encodeResult = pb_encode (&myStream, request_fields, &myRq);
  messageLength = myStream.bytes_written;

  Serial.println("request:");
  Serial.println(encodeResult);
  Serial.println(messageLength);
  Serial.println((char*) buffer );

  request decodedRq = request_init_default;
  pb_istream_t iStream = pb_istream_from_buffer(buffer, messageLength);
  pb_decode(&iStream, request_fields, &decodedRq);

  Serial.println(decodedRq.client_id);
  Serial.println(decodedRq.operations_count);


}

void setup() {
  display.init();
  Serial.begin(115200);

  displayMyMac();
  //testChacha();
  //testProtobuffResp();
  testProtobuffRq();
}

void loop() {
    delay(1000);
}