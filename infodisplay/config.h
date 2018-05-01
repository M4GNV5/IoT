#pragma once

//uncomment if you want output on the serial interface
//#define DEBUG

#define WIFI_SSID "<ssid>"
#define WIFI_PSK "<password>"

#define IMAGE_SOURCE_HOST "<server domain or ip>"
#define IMAGE_SOURCE_PORT 80
#define IMAGE_SOURCE_PATH "/infodisplay.dat"

//refresh cycle in seconds
#define REFRESH_CYCLE (3 * 60)