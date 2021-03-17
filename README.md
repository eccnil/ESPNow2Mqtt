Motivation
==========

If you need to create a battery powered device with esp32, then mqtt looks great but connecting to the WiFi could take 2 seconds of high power consumption, add to that the fact that polling for a subscription can last other 2 seconds adn the result is your battery drained doing connection stuff.

With this library you can comunicate with the mqtt server within 100 ms. 

You can build both, the device part and the gateway. Some examples of both.

Architecture
============

Overview
--------

The idea is letting you making your own device and connecting it to mqtt via gateway:

![big picture](https://www.plantuml.com/plantuml/png/VP8nJyCm48Nt-nMdR0mPA5iLHPMWOg8WCR0W8MCVgQN45-mpLONuxpXEMhK4rPFzxjtJToUlWYWliMlntJROMotH4u1rGlv3QS90Bh2_dYcB4qMbBYNiRm0S6qBBSaTVLjCtO3gQkvuTES5Ypz6djw42LS0utFvOrg7Vy6xny2boszgav7fsObMMzToGol03C87YLLbEmHFeYN84nQCq4ZTAQAVsU0cughWNoqwrTJtGDvVfxyV8R6WE-FyP4ljyfkfyqicBIrm_Fxrj7by0TClHOC6o9OzTyhmwH9FN7l9snyfP-25cI1_BozmAi29dqY_u1G00 "big picture")

Therefore this library provides two (singleton) objects. In the *iotDevice* you use `EspNow2MqttClient`to send the data (ie. your sensor reads) and also to gather data (polling) from mqtt (ie. commands for your actuators)

You do not need to put code in your gateway unless you need to show info in a display or something similar, as is done in the [display server example](examples/server/server.cpp)

Design
------

ESP-Now has some limitations that influenced our design:

![internals](https://www.plantuml.com/plantuml/png/fP8nJyCm48Lt_ufJbqpjm8YYg1Agc2YLEZ3RZabEScnZNwW2wd_d4gSjHHK3M5vyVkzyxvFNYMIalhE0ZanqqYPHMFDAG1uYDYr1ZT5eGk-448NpjG5jqJY2Jm6yjE-2T-DvayBM7-wUteWU9aKMN8kST7wde2mLEXa7I2QYjFWXnnUg2cIQ947f6sRTIjviJaDdAA86FOIMExWcMxFWOfrRGRbQLLUNHr4SJt4QIUOWXz_oF6R9hK2XL_jU9HrH6PwiOpLOL973vPd62XV-mwBvEukBTPuZi_ZPTpv_1zNCe2PICpieb-VhUzZiTyljyuiG4yR559qplACsYrDo8SxliZLNriRL_EM-0000 "internals")

You cannot connect more than 6 nodes if you use ESP-Now with cyphering. To avoid this we have decided to not using it and using our own cyphering with a software layer based on Chacha (as LoraWan does). 

*The idea was taken from [EnigmaIOT](https://gmag11.github.io/EnigmaIOT/html/index.html) but we decided to build from scrach because our goal is to provide a library instead of a framework where you can place your code in.*

You have a very simple protocol defined with nanopb (*protocol buffers for iot*) that allows to query and write to many mqtt topics with a simple message. Here you can see the [definition](messages.proto) and [limitations](messages.options). But you don't need to kown the details you can use the helper functios as detailed in the [examples folder](examples/client)

Using this simple protocol ensures minimial overhead waste, but remember: ESP-Now limits your messages to 250 bytes, keep your messages low (we capped the payload to 200 bytes and 5 pub/subs per request)

Mqtt topics:
-----------

As there is a message size cap, is not sensible to use full canonical topics names, because they whould consume all the available bandwith.

Therefore you only need to especify the las part (call it the name). You can find your data in a topica called:

EspNow/clientId/name

Where

- EspNow is a fixed literal (you can modify in this [header](src/EspNow2MqttGateway.hpp))
- clientId is the name you are required when creating your instance of `EspNow2MqttClient`. If various devices share this ID then they will share its topics without need of a third party
- name is your choice for any given message (publication or subscrition)


Simplicity: our *blink* version
----------

Sf you look at the examples folder you may see lots of lines of code. But simplicity is a goal. If you don't want to have fancy features you can have as little source as this:

```c++
#include <Arduino.h>
#include <EspNow2MqttClient.hpp>

#define LED 2

byte sharedKey[16] = {10,200,23,4,50,3,99,82,39,100,211,112,143,4,15,106};
byte sharedChannel = 8 ;
uint8_t gatewayMac[6] = {0xA4, 0xCF, 0x12, 0x25, 0x9A, 0x30};
EspNow2MqttClient client = EspNow2MqttClient("testLib", sharedKey, gatewayMac, sharedChannel);

void setup() {
  pinMode(LED, OUTPUT);
  client.init();
}

void loop() {
  digitalWrite(LED, HIGH);
  client.doSend("ON","led");
  delay(1000);
  digitalWrite(LED, LOW);
  client.doSend("OFF","led");
  delay(1000);
}
```


Client
======

as a client you need to add this dependence to you program:

```c++
#include <EspNow2MqttClient.hpp>
```

follow the examples on the examples folder

you can send 4 types of messages:

1. ping : testing message, does not reach to mqtt, only tests espnow
2. send : deliveres a message to the queue of your choice
3. subscribe: gives the content of a queue when it changes. this message creates a subscription in the gateway, when the gateway gets the message stores it and delivers to you on the next 'subscribe' message with the same queue
4. multiple: any combination of up to 10 atomic messages (ping, send or subscribe). the response will be an array with the same order

remember anycase that you cannot exced 200 bytes as an esp-now limitation

Server
======

You can build gateway server just taking the example as is. Or include this file and build your own:

```c++
#include <EspNow2MqttGateway.hpp>
```

You need to share the key, channel and mac with your clients in order to allow only YOUR clients to connect.

- channel: your ESP cannot use two channels, Therefore you need to configure your esp-now channel to coincide with your WiFi network. All your clients need to know and use this channel. On the sleeping client you can find how to get the channel of your WiFi
- key: all mensages are ciphered. use same key in all clients and the gateway. As the ciphered is performed by software there is no limit in the number of clients.
- mac: all the clients need to know the mac of the gateway. In the example you can find a function to get it printed

Versioning
==========

v1.1
----

- message format
  - few operations on a batch (5) to save memory on the stack
  - messageId: a new (optional) field on requests and responses that the gateway will return to you to ease response recognition, it an be used
    - as a correlation id
    - as a message type id
    - ignored
- gateway parametrizable identification, to allow having multiple gateways at same time (and extend your network reach)

v1.2
----

- callback when mqtt data is received
- methods
  - `int getNumberOfSubscriptions()` gives the number of mqtt topics being listened
  - `int getNumberOfMessages()` gives the number of messages fetched and not (yet) delivered

v1.3
---

- sendGwMqttMessage function that allows to publish internal gw user info (for example to send the chip temperature)

v1.3.1
---

- resubscribe to all topics when reconnectig to mqtt server (lost of connection)


Miscelanea
===========

### actual projects using the library

a brief list (building) of complete projects using the library

- [home esp-now gateway](https://bitbucket.org/enrique_vicent/homeespnowgateway/src/master/), a gateway for more devices
- [airCond controller](https://bitbucket.org/enrique_vicent/aire-acondicionado/src/master/), a battery powered button pusher 
- [eInk desktop monitor](https://bitbucket.org/enrique_vicent/statusmonitor)


### note on dependencies

sometimes the dependencies are not resolved by itself add this manually if happens to you:

```
Crypto@^0.2.0
Nanopb
knolleary/PubSubClient@^2.8
```

### To compile the libary itself

+ [regenerate protobuf](documentation/protobuf.md)
+ use platform.io and select the profile you need
