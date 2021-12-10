User Manual
==========

Description
------------

A client is any device that connectes to the gateway using esp-now protocol.

A client is suposed to be a battery powered device becasuse esp-now has many advantages in power management when compared with connecting to mqtt directly. The most important feature is the **low time to connect** and transmit data. 

Tipical application using wifi and mqtt uses about 4s to query a mqtt queue: about 2 seconds to connect to the access point and about 2 more seconds to connect to the mqtt server and query a topic. 

Using espnow2mqttGateway the communication can be less than 10 milliseconds (5-6 tipically) that enables a ESP32 to wake, query a queue and go back to sleep within 20 milliseconds (16 tipically) and thats is a huge improve in energy saving.

Messages
----------

### message

A request message is composed up to 5 commands.

You can send a single command per message but in that case the communication is slower and you battery may long less.

The response to a message has the same number of command-responses that the original message, and in the same order.

#### messageType

MessageType is a number (int) that you can set in a request message and you will receive it back in the response message. It has 3 possible usages

- be ignored, in that case simply skip the data (will be 0)
- use to recongnice the message type in order to process the response
- use to correlate with the request.

The original use of messageType is to differenciate the message type in case that you send various types of messages. 

That is common practice when you have more than 5 commands to send, or if you choose to launch then one by one. You can assign 1 for writing the first 5 values, 2 for writing 5 more and 3 to queryng a value. Then, when processing responses you know thant the messageType=3 command=0 is the value you queried

But you can use it as a [correlationId](https://docs.mulesoft.com/mule-runtime/4.4/correlation-id). You can also use a Ping command for that

### commands

there are 3 kinds of commands

1. ping
2. send
3. response

Each one is defined in this [proto](messages.proto) file and the field limites in this [options](messages.options) file. 

#### ping

ping allows you to test communication with the gateway (mqtt not involved), you send a number and the number comes back. This command can be used also as a correlationId.

#### send

send allows to write a message into a mqtt topic
you need to specify:

- **queue** : the mqtt queue (or topic, we use both terms meaning the same) to write the message to. If the topic does not exists, then is created as a second level of a topic with the same name of the wateway mqtt `client_id`.
- **payload** : data to be sent o the topic, always in string format. if you need a (int)1 just send "1" 
- **persist** : if you want to write the message with the persist flag in mqtt. *warning currently not implemented feature*

#### subscribe

subscribe tells the gateway that you are interested in any message in the specified queue. 

take into account that the gateway will use the first 'subscribe' message for a topic, to register to that queue. Therefore the first subscribe will give no information and the response code will be `NO_MSG` no older messages will fetched (unless marked as persistent)

a subscribe operation command is composed of:

- **queue** : the mqtt queue (or topic, we use both terms meaning the same) to get data from
- **remove** : unused data.


### response

all commands receive the same structure as response 

- **result code** one of 
  - ok: `ping` ok, `send` ok, or `subscribe` ok and has returned data
  - no_msg: only for `subscribe` command, tells that the operation was ok but no new data was found in the topic
  - nok: error
  - nomqtt: error related to being unable to connect with mqtt, only for `send` command
- **payload** in case of resultcode=ok 
  - for `ping`: the number you send, back to you
  - for `send`: empty
  - for `subscribe`: the data on the queue that was not sent in any former invocation


The basics
--------------

### setup connection

you need 3 pieces of data from the [gateway](GatewayManual.md)

- the same sharedKey you set in the gateway
- the same channel you are using in the gateway
- the mac address of the gateway

#### calculate wifi channel on run time

wifi access points does not use to change the channel, but if you do so for any reason it will be a serious inconvenience to recompile and re-deploy all your devices. For that reason is advised to calculate the channel each time the system boots up. If you did so a AP channel change require just reseting your devices. 

This operation is similar to the gateway but if you perform this operation each time then you won't achieve the low connection times the library promises.

If you are deep sleeping, then the recommended way is to fetch channel only on boot up time, store in the ulp and skip the step every time you wake-up (not applies to esp8266, use fixed channel instead)

this function can do the job

```c++
void getChannel(char * ssid , itn & shared_channel){
    switch(esp_sleep_get_wakeup_cause()){
        case ESP_SLEEP_WAKEUP_EXT0 : break;  
        case ESP_SLEEP_WAKEUP_EXT1 : break; 
        case ESP_SLEEP_WAKEUP_TIMER : break; 
        case ESP_SLEEP_WAKEUP_TOUCHPAD : break;
        case ESP_SLEEP_WAKEUP_ULP : break;
        default: //update channel only on cold boots
            if (int32_t n = WiFi.scanNetworks()) {
                for (uint8_t i=0; i<n; i++) {
                    if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
                        shared_channel = WiFi.channel(i);
                    }
                }
            } 
        break;
    }
}
```

#### getting the mac 

With this implementation the client macs are irrelevant but you need to know the mac address of the gateway. 

Each wi-fi chipshet has a unique number since its manufacturation, that is called the mac address. 

You can get the mac address in the format you need using this function (in the gateway)

```c++
void displayMyMac(){
    char macStr[22];
    strcpy(macStr, "Mac ");
    strcat(macStr,WiFi.macAddress().c_str());
    Serial.println(macStr);
}
```

## completing the setup

there are 2 things you must place in the correct places

``` c++
#include "EspNow2MqttClient.hpp"

//outside any function: the object declaration 
`EspNow2MqttClient client = EspNow2MqttClient("myName", sharedKey, gatewayMac, sharedChannel);`

//in the setup()
void setup(){
    client.init();
} 
```

you can provide init with the espnow channel but as described in the gateway manual, if you dont need an especific espnow channel leave it as it is (0)


## sending basic messages

basic messages are messages that contains only a command. Useful for basic scenarios. But more complex when you have advanced scenarios. The following functions send a message with a single command on it. Take into account that thansks to protobuff, the message is as short as strictly needed, therefore the single messages are very short.

- `doPing`
- `doSend`
- `doSubscribe`

those function wont be described because they compose the message as already described and send it. You should process the response (if you care about the response) using a callback function `onProcessedRequest``

*note: if you use `doPing` you cannot set the number you want to see coming back, it sents the number of thimes you used doPing (auto incremented) this function is intended to ease the test of the range of espnow (display needed) using [ping example](examples/client/pingBasicClient/basicPingClient.cpp)*

### the `onProcessedRequest` callback

This callback function allows to process the gateway responses in basic and advanced message patterns.

this can be defined like (use any name):

```c++
void myOnProcessedRequestCallback( response & rsp){...}
```

don't forget to assign the function to the client like

```c++
client.onReceiveSomething = myOnProcessedRequestCallback;
```

this function receives a response as described before in the document. 

for basic messages just check that `rsp.opResponses[0].result_code == response_Result_OK``

for complex messages the recomended approach is to create a decission tree based on 

1. message type number (those you define)
2. command index (from 0 to opResponses_count-1)
3. command resultCode

see [examples](examples/client)

Advanced scenarios
--------------------

### multiple-command messages

If you need to send multiple commands on a single messages (recommended) or if you want to save the expense process of creating the complete messages each time then this is your section

First of all you need to create an empty message. This message will contain 5 commands but will send only those we fulfill. This library optimizes bandwith usage but not memory usage at this point.

Create an empty message with `createRequest()`. Its a good optimization to create in the decalaration area (outside any function) and reuse it, otherwise is easy to deplete the stack area of the memory. All related functions are declared as inline to preserve the stack as much as possible.

you can create commands with `createRequestOperationPing`, `createRequestOperationSend` and `createRequestOperationSubscribeQueue` methods.

then you can pace those commands in the `request.operations` array and update the `operations_count` field to tell the library how many messages are fulfilled. Place the commands in order, first the [0] then the [1] and so on.

To send this message use the method `doRequest`

an example is available at the `testMultipleRequests` function of the [sleepingClient example](examples/client/sleepingClient/sleepingClient.cpp)

### on sent ack

the function on sent ack is intended for those scenarios consisting on a sensor sending data. If you dont care about the mqtt ack and you just want to know if the gateway received the message you can use `onSentAck` callback instead of `onReceiveSomething` callback

The transmision time is cut to half because the gateway sends the ack before  processing the message. But you can still know if the message has arrived or not. Ignoring this callback saves no transmision time since the espnow waits for this dataframe anyway. 

The only parameter that you will recieve is a boolean parameter indicating sucess or not. If false, the mesagge can be delay up to 100 ms, depending the reason. 





