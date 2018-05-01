#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "VS1053.hpp"
#include "config.h"

//built-in led on LoLin boards is off when D4 is HIGH
#define DEBUG_LED_PIN D4
#define DEBUG_LED_ON LOW
#include "common/Blink.hpp"

#define HTTP_USER_AGENT "WebRadio ESP8266/VS1053 - https://github.com/M4GNV5/IoT"
#include "common/HTTP.hpp"

#include "common/Log.hpp"
#include "common/ConnectionManager.hpp"

WiFiClient mqtt_sock;
PubSubClient mqtt(mqtt_sock);

void mqttReconnect();
ConnectionManager connections(mqtt, mqttReconnect);

WiFiClient httpInput;
WiFiClientSecure httpsInput;

WiFiClient *input = NULL;
VS1053 output(VS1053_PINS);

bool playing = false;
uint8_t volume = 70;
char stationUrl[1024];

uint8_t *buffer;
size_t buffpos;
size_t bufflen;

#define ERROR(code, msg) do { \
		LOGLN(msg); \
		blink(code); \
	} while(0)

static void startPlaying(const char *url)
{
	if(playing)
		stopPlaying();

	LOG("Playing ");
	LOGLN(url);

	const char *host;
	uint16_t port;
	if(strncmp(url, "http://", 7) == 0)
	{
		input = &httpInput;
		port = 80;
		host = url + 7;
	}
	else if(strncmp(url, "https://", 8) == 0)
	{
		input = &httpsInput;
		port = 443;
		host = url + 8;
	}
	else
	{
		ERROR(4, "Invalid URI");
		return;
	}

	const char *path = strchr(host, '/');
	if(path == NULL)
	{
		ERROR(4, "Not a valid http path");
		return;
	}

	int hostLen = path - host;
	char _host[hostLen + 1];
	memcpy(_host, host, hostLen);
	_host[hostLen] = 0;

	if(path[0] == 0)
		path = "/";

	if(http_get(input, _host, port, path))
	{
		output.startSong();
		buffpos = 0;
		bufflen = 0;
		playing = true;
		LOGLN("Successfully started playing");
	}
	else
	{
		blink(5);
	}
}
static void stopPlaying()
{
	if(!playing)
		return;

	LOGLN("Stopping playback");

	if(input->connected())
	{
		input->stop();
		input->flush();
	}

	output.stopSong();
	playing = false;
}

static unsigned strntou(char *str, size_t len, char **endptr)
{
	unsigned val = 0;
	size_t i;

	for(i = 0; i < len; i++)
	{
		if(str[i] < '0' || str[i] > '9')
			break;

		val = val * 10 + str[i] - '0';
	}

	if(endptr != NULL)
		*endptr = str + i;

	return val;
}

static void sendStatus()
{
	if(playing)
	{
		char *buff = (char *)malloc(strlen(stationUrl) + 7);
		sprintf(buff, "1,%d,%s", (int)volume, stationUrl);

		mqtt.publish(MQTT_TOPIC "/status", buff, true);
		free(buff);
	}
	else
	{
		mqtt.publish(MQTT_TOPIC "/status", "0", true);
	}
}

void mqttReconnect()
{
	mqtt.subscribe(MQTT_TOPIC "/playing");
	mqtt.subscribe(MQTT_TOPIC "/volume");
	mqtt.subscribe(MQTT_TOPIC "/tone");
	mqtt.subscribe(MQTT_TOPIC "/station");

	sendStatus();
}

void mqttCallback(char *topic, uint8_t *payload, unsigned len)
{
	LOG("Received MQTT message on ");
	LOG(topic);
	LOG(" of length ");
	LOG(len);
	LOG(": ");
	for(unsigned i = 0; i < len; i++)
		LOG((char)payload[i]);
	LOGLN("");

	unsigned prefixLen = strlen(MQTT_TOPIC);
	if(strncmp(topic, MQTT_TOPIC, prefixLen) != 0)
		return;

	topic += prefixLen;

	if(strcmp(topic, "/playing") == 0)
	{
		bool newPlaying;

		if(len == 1 && payload[0] == '1')
			newPlaying = true;
		else if(len == 1 && payload[0] == '0')
			newPlaying = false;
		else
			return;

		if(playing == newPlaying)
			return;
		else if(newPlaying)
			startPlaying(stationUrl);
		else
			stopPlaying();

		sendStatus();
	}
	else if(strcmp(topic, "/volume") == 0)
	{
		uint8_t oldVolume = volume;

		char *end;
		volume = strntou((char *)payload, len, &end);
		if(end != (char *)payload + len)
			return;

		if(volume > 100)
			volume = 100;
		if(volume != oldVolume)
		{
			LOG("Set volume to ");
			LOGLN(volume);

			output.setVolume(volume);

			if(playing)
				sendStatus();
		}
	}
	else if(strcmp(topic, "/tone") == 0)
	{
		uint8_t values[4];
		char *pos = (char *)payload;
		for(int i = 0; i < 4; i++)
		{
			values[i] = strntou(pos, len - (pos - (char *)payload), &pos);

			if(i != 3 && (pos >= (char *)payload + len || *pos++ != ','))
				return;
		}

		LOG("Setting tone to ");
		for(int i = 0; i < 4; i++)
		{
			LOG(values[i]);

			if(i != 3)
				LOG("/");
		}
		LOGLN("");

		output.setTone(values);
	}
	else if(strcmp(topic, "/station") == 0)
	{
		if(len > 1023 || (playing && strncmp(stationUrl, (char *)payload, len) == 0))
			return;

		memcpy(stationUrl, payload, len);
		stationUrl[len] = 0;
		startPlaying(stationUrl);

		sendStatus();
	}
}

void setup()
{
#ifdef DEBUG
	Serial.begin(115200);
	delay(1000);
#endif
	LOGLN("");

	WiFi.persistent(false);
	WiFi.mode(WIFI_OFF);
	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PSK);

	mqtt.setServer(MQTT_SERVER, MQTT_PORT);
	mqtt.setCallback(mqttCallback);

	connections.lastWillRetain = true;
	connections.lastWillTopic = MQTT_TOPIC "/status";
	connections.lastWillMessage = "0";

	SPI.begin();
	output.begin();
	output.switchToMp3Mode();
	output.setVolume(volume);

	buffer = (uint8_t *)malloc(BUFFER_SIZE);
	buffpos = 0;
	bufflen = 0;

	stationUrl[0] = 0;
}

void loop()
{
	connections.loop();

	if(playing)
	{
		if(!input->connected())
		{
			ERROR(5, "Connection to server lost");

			playing = false;
			startPlaying(stationUrl);

			if(!playing)
				return;
		}

		size_t bytesAvailable = input->available();
		if(bufflen < BUFFER_SIZE && bytesAvailable > 0)
		{
			uint8_t *start = &buffer[(buffpos + bufflen) % BUFFER_SIZE];
			uint8_t *buffend = buffer + BUFFER_SIZE;

			size_t len = bytesAvailable;
			if(start + len >= buffend)
				len = buffend - start;
			if(BUFFER_SIZE - bufflen < len)
				len = BUFFER_SIZE - bufflen;

			input->readBytes(start, len);
			bufflen += len;
		}

		while(bufflen >= TRANSFER_CHUNK_SIZE && output.data_request())
		{
			//TODO make sure buffer + buffpos + TRANSFER_CHUNK_SIZE doesnt exceed buffer + BUFFER_SIZE
			//as long as BUFFER_SIZE is a multiple of TRANSFER_CHUNK_SIZE this shouldnt be a problem
			output.playChunk(buffer + buffpos, TRANSFER_CHUNK_SIZE);

			buffpos = (buffpos + TRANSFER_CHUNK_SIZE) % BUFFER_SIZE;
			bufflen -= TRANSFER_CHUNK_SIZE;
		}
	}

	delay(0);
}
