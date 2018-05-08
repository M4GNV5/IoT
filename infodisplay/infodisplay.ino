#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <GxEPD.h>

#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>
#include <GxGDEW075T8/GxGDEW075T8.cpp>

#include "config.h"

//built-in led on LoLin boards is off when D4 is HIGH
#define DEBUG_LED_PIN D4
#define DEBUG_LED_ON LOW
#include "common/Blink.hpp"

#define HTTP_USER_AGENT "Infodisplay - https://github.com/M4GNV5/IoT"
#include "common/HTTP.hpp"

#include "common/Log.hpp"

GxIO_Class displayIO(SPI, D8, D1, D6);
GxEPD_Class display(displayIO, D6, D2);

WiFiClient input;

#define MAGIC 0xdeadbeef
struct
{
	uint32_t magic; //if this ==MAGIC we know we've written to the RTC memory before
	uint32_t localIP;
	uint32_t gateway;
	uint32_t netmask;
	uint32_t dns0;
	uint32_t dns1;

	int32_t channel;
	uint8_t bssid[6];

	char lastModified[64];
} config;

bool newLastModified = false;
bool imageModified = true;

#define LAST_MODIFIED_HEADER "Last-Modified"
void header_handler(char *buff, bool isComplete)
{
	size_t headerLen = strlen(LAST_MODIFIED_HEADER);
	if(strncmp(buff, LAST_MODIFIED_HEADER, headerLen) != 0)
		return;

	buff += headerLen + 2; //skip header, : and space

	if(strncmp(buff, config.lastModified, 64) == 0)
	{
		imageModified = false;
	}
	else
	{
		strncpy(config.lastModified, buff, 64);
		newLastModified = true;
	}
}

void goToSleep()
{
	LOG("Going to sleep after ");
	LOG(millis());
	LOGLN("ms");

	int32_t sleepMs = (REFRESH_CYCLE * 1000) - millis();
	if(sleepMs <= 0)
		ESP.restart();
	else
		ESP.deepSleep(sleepMs * 1000);
}

void setup()
{
#ifdef DEBUG
	Serial.begin(115200);
	delay(1000);
#endif
	LOGLN("");

	display.init();

	pinMode(DEBUG_LED_PIN, OUTPUT);
	digitalWrite(DEBUG_LED_PIN, HIGH);

	WiFi.persistent(false);
	WiFi.mode(WIFI_OFF);
	WiFi.mode(WIFI_STA);

	LOG("Connecting to WiFi ");
	LOG(WIFI_SSID);
	if(ESP.rtcUserMemoryRead(0, (uint32_t *)&config, sizeof(config))
		&& config.magic == MAGIC)
	{
		LOG(" using static configuration");

		WiFi.begin(WIFI_SSID, WIFI_PSK, config.channel, config.bssid, true);
		WiFi.config(config.localIP, config.gateway, config.netmask, config.dns0, config.dns1);
	}
	else
	{
		LOG(" and configuring via DHCP");

		WiFi.begin(WIFI_SSID, WIFI_PSK);
		config.magic = MAGIC + 1;
	}

	for(uint8_t i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++)
	{
		LOG(".");
		blink(1);
		delay(800);
	}
	LOGLN("");
	
	if(WiFi.status() != WL_CONNECTED)
	{
		LOGLN("Connecting to WiFi failed");
		blink(2);
		goToSleep();
	}
	else if(config.magic != MAGIC)
	{
		config.magic = MAGIC;
		config.localIP = WiFi.localIP();
		config.gateway = WiFi.gatewayIP();
		config.netmask = WiFi.subnetMask();
		config.dns0 = WiFi.dnsIP(0);
		config.dns1 = WiFi.dnsIP(1);

		config.channel = WiFi.channel();
		memcpy(config.bssid, WiFi.BSSID(), 6);

		config.lastModified[0] = 0;

		ESP.rtcUserMemoryWrite(0, (uint32_t *)&config, sizeof(config));
	}

	if(!http_get(&input, IMAGE_SOURCE_HOST, IMAGE_SOURCE_PORT, IMAGE_SOURCE_PATH, header_handler))
	{
		LOGLN("HTTP GET failed");
		blink(2);
		goToSleep();
	}

	if(!imageModified)
	{
		input.stop();
		input.flush();

		LOGLN("Image did not change");
		blink(1);
		goToSleep();
	}

	if(newLastModified)
	{
		LOG("Got new Last-Modified value: ");
		LOGLN(config.lastModified);

		ESP.rtcUserMemoryWrite(0, (uint32_t *)&config, sizeof(config));
	}

	LOGLN("Receiving Image...");

	uint16_t x = 0;
	uint16_t y = 0;
	size_t bytesAvailable = 0;
	while(y < GxEPD_HEIGHT)
	{
		do
		{
			yield();
			bytesAvailable = input.available();
		} while(bytesAvailable == 0 && input.connected());

		if(!input.connected())
			break;

		for(; bytesAvailable > 0; bytesAvailable--)
		{
			uint8_t byte = input.read();
			for(int8_t i = 7; i >= 0; i--)
			{
				display.drawPixel(x, y, (byte & (1 << i)) ? GxEPD_BLACK : GxEPD_WHITE);

				x++;
				if(x == GxEPD_WIDTH)
				{
					x = 0;
					y++;
				}
			}

			yield();
		}
	}

	LOGLN("Updating Display...");
	display.update();

	blink(1);
	goToSleep();
}

void loop()
{
	//never reached
	delay(10000);
}
