# Plant Watering System

Simple project that sends data from an Arduino to an ESP8266 via UART. 
The ESP8266 forwards the data to a server which reads data and controls the Arduino using MQTT.

The following libraries need to be installed:

* https://github.com/esp8266/Arduino - Arduino platform support for ESP8266
* https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib - Temperature sensors library
* https://bblanchon.github.io/ArduinoJson/ - easy JSON data encapsulation
* https://github.com/knolleary/pubsubclient - MQTT library

For the MQTT library, change PubSubClient.h the following macros to:

* #define MQTT_MAX_PACKET_SIZE 254
* #define MQTT_MAX_TRANSFER_SIZE 254 /* uncomment this one as well */

This will enable longer payloads in MQTT messages.
