#pragma once

//uncomment if you want output on the serial interface
//#define DEBUG

#define WIFI_SSID "<ssid>"
#define WIFI_PSK "<password>"

#define MQTT_SERVER "<mqtt server domain or ip>"
#define MQTT_PORT 1883

#define MQTT_CLIENT_ID "radio0"
#define MQTT_TOPIC "home/radio0"

//cs, dcs, dreq
#define VS1053_PINS D1, D0, D2

#define BUFFER_SIZE 32768
#define TRANSFER_CHUNK_SIZE 32
