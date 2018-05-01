# IoT

IoT stands for Internet of Things, i.e. "smart" and "connected" devices. Usually this boils
down to having an app which connects to a server in China which allows you control a
device (i.e. coffe machine, wall socket, radio, light, ...) which connects to a server in China
too. This repository contains code which runs at my home to make it "smart" and "connected"
without connecting to China.

## Contents

- The [common](common/) directory contains code which is used in multiple projects (e.g. managing WiFi and MQTT connections)
- [A custom software](sonoffsocket/) for the [Sonoff S20](https://www.ebay.de/itm/Sonoff-S20-WIFI-Smart-Power-Socket-Wireless-Remote-Control-Timer-US-EU-Plug-P9M/263383861102?ssPageName=STRK%3AMEBIDX%3AIT&var=562394786094&_trksid=p2057872.m2749.l2649)
- [A webradio](radio/) using a [NodeMCU kit](https://www.ebay.de/itm/ESP8266-CH340G-NodeMcu-V3-Lua-NodeMCU-Breakout-Expansion-Board-Development-Board/382289678553?ssPageName=STRK%3AMEBIDX%3AIT&var=651099378345&_trksid=p2057872.m2749.l2649) and a [VS1053](https://www.ebay.de/itm/VS1053-MP3-Module-Development-Board-w-On-Board-Recording-Function-SPI-Interface/310645105092?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649) (includes a [VS1053 library](https://github.com/baldram/ESP_VS1053_Library) taken from [Marcin Szałomski](https://github.com/baldram))
- [A WiFi connected ePaper display](infodisplay/) which loads images to display on [the screen](https://www.amazon.de/Makibes-Touchscreen-Electronic-Zwei-Farben-Controller/dp/B0773KKQVV) from a webserver