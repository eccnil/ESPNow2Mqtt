Gateway Manual
==========================

Description
---------------------------

The gateway has two connections 

1. a wi-fi / mqtt
1. a espnow

the job of the gateway is to receive messages espnow and translate to mqtt messages or actions. there are 3 kinds of messages

1. **ping** does nothing in mqtt leg of the communication
2. **send** sends the message to the defined mqtt queue
3. **subscribe** first time subscribes to a mqtt queue, further subscriptions get the **last** message on the queue.

this messages (also called operations) can be stacked on a single request (max 5) to better performance on communications.

the gateway will collect the result status for each kind of message (operation) and reply back to client with the same stack order an size. see [messages definitions](messages.proto) and [messages parameters](messages.options) for more details


Basics
---------------------------

Most people does not want to do nothing fancy on its wateway. For this reason the code on (examples/server/server.cpp) is fully functional.

You can copy its content in your main.cpp and the gateway should work. But some changes are recommended

- if you dont have connected an spi display create the display objet with (false) and the debug information will be produced on the serial port. (you can remove the object, is not needed for the gateway)
- esp-now conection params 
  - sharedKey: *chacha* data encription key for the *esp-now* communication, choose 16 ramdom values. Use the same key in all the devices connected to this wateway
  - sharedChannel: you wifi (radio) channel. Because switching between channels will cause lost of messages while the wateway is not in the message channel. The best option is to calculate the channel on run-time but this value will serve as a default value if wifi is down, more details following.
  - esp-now channel. default = 0, recommended leaving 0 for "auto"
- mqtt connection params, see below.
- gateway name

### calculate wifi channel on run time

wifi access points does not use to change the channel, but if you do so for any reason it will be a serious inconvenience to recompile and re-deploy all your devices. For that reason is advised to calculate the channel each time the system boots up. If you did so a AP channel change require just reseting your devices. 



```c++
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
const char * ssid = "myWifiId";
byte sharedChannel = 8; 

....

if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i=0; i<n; i++) {
        if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
            sharedChannel = WiFi.channel(i);
        }
    }
} 
```

### mqtt connection params

as you can see the gateway expects the Wifi connection already set.

```c++
WiFiClient wifiClient;
EspNow2MqttGateway gw = EspNow2MqttGateway(sharedKey, wifiClient, MQTT_SERVER_IP, 1883, sharedChannel, "gardenGW");
```

This is so to allow you connecting to your wifi with your favorite method. you can even setup the esp32 as a hotspot. Therefore the parameters for connecting to your wifi are not discussed in this document, but obviously you need to setup them also.

be sure to connect to your wifi before initing EspNowGateway, or the system will start re-trying early.

the other parameters you need to connect mqtt are:

- mqtt server ip: the ip of the mqtt server
- mqtt server port, defaults to 1883
- gateway name, defaults to *EspNow* sets the name of the client in mqtt, but also is the root topic for all the topics used by clients. 
- mqtt username, defaults to nothing (no auth). if you have security in mqtt set here the name
- mqtt passeord, defaults to nothing (no password). if you have security in mqtt set here the password

### basic api

* `init`: function intended to be placed in the arduino `setup` function. setups all the needed connections. It asumes wifi is already connected therefore is a good idea to use this method after initialisation of the wifi connection
* `loop`: function intended to be placed in the arduino `loop` function. it polls and proceses both mqtt and esp-now communication legs.

More functionality
------------------

if you want to add more functionality there are some things you may use:

## callbacks

### `onProcessedRequest`

this callback is called when an esp-now request has been procesed (therefore is called **after** being processed, and introduces no delay)

declare it like:

```c++
void displayRequestAndResponse(bool ack, request &rq, response &rsp ){
```

- ack, indecates if the ack of the response has been received
- request: the original request
- response: the response the gateway sent back

asign it like:

```c++
gw.onProcessedRequest = displayRequestAndResponse;
```

### `onMqttDataReceived`

this callback is called anytime valid data is received by mqtt, declare it like:

```c++
void displayMqttIncomingData(char* topic, byte* payload, unsigned int length){
```

- topic: subscription topic where the data arrived from. The subscription need to be made as a pre-requisite to receive data.
- payload: data received
- lengt: of data received, mind that espnow messages has a limit, message may be trunked.

assign it like:

```c++
gw.onMqttDataReceived = displayMqttIncomingData;
```

### `onDataReceived`

this function is similar to onMqttDataReceived but for the espnow side. Caution it is processed between the receive and the response and the total roundtrip time will include the processing time of this function. Using Serial.println is discouraged because will impact the client performance.

## last will queue

everytime the gateway lost its connection with mqtt, the message "EspNow2MqttBridge died!" will appear in the last will queue.

this queue name is composed of the mqtt client name (optionally provided during object creation) and the "_will" postfix.

the gateway will start to re-connect as soon as the connection has been noted to be down. And all subscriptions will be recreated as soon as re-connection succed

## `sendGwMqTTMessage``

this method is used to publish data to a mqtt topic in name of the gateway itself. 

it has the ability to publish on a topic of other client and the data will be sent to both the mqtt broker and the client (when client requires)

its intention is to publish internal data of the gateway. If you decide to install some sensors, or monitor some parameter (ie wifi strength) the gathered data can be sent with this method

some internal data you may want to send are:

- getNumberOfSubscriptions() : how many topics the gateway is subribed to. The gateway subscribes to the topics in name of the clients
- getNumberOfMessages() : number of messages temporary stored to be delivered to the clients when they ask for. the max value is getNumberOfSubscriptions()



