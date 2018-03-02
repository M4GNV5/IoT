#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "VS1053.hpp"
#include "config.h"
#include "ConnectionManager.hpp"

WiFiClient mqtt_sock;
PubSubClient mqtt(mqtt_sock);

//built-in led on LoLin boards is off when D4 is HIGH
ConnectionManager connections(mqtt, D4, HIGH);

WiFiClient input;
VS1053 output(VS1053_PINS);

bool playing = false;
uint8_t volume = 70;
char stationUrl[1024];

uint8_t *buffer;
size_t buffpos;
size_t bufflen;

//very simplistic HTTP get
bool http_open(const char *host, const char *path)
{
	if(!input.connect(host, 80))
	{
#ifdef DEBUG
		Serial.print("Coult not connect to ");
		Serial.println(host);
#endif
		return false;
	}

	input.print("GET ");
	input.print(path);
	input.println(" HTTP/1.1");

	input.print("Host: ");
	input.println(host);

	input.println("User-Agent: WebRadio ESP8266/VS1053 - https://github.com/M4GNV5/IoT");
	input.println("Accept: audio/*");
	input.println();

	const char *httpOk = "HTTP/??? 200 OK\r\n";
	const int httpOkLen = strlen(httpOk);
	for(int i = 0; i < httpOkLen; i++)
	{
		while(input.connected() && !input.available())
			yield();

		if(!input.connected() || (input.read() != httpOk[i] && httpOk[i] != '?'))
		{
#ifdef DEBUG
			Serial.println("Unexpected HTTP response");
#endif
			input.stop();
			input.flush();
			return false;
		}
	}

	bool startOfLine = true;
	for(;;)
	{
		while(input.connected() && !input.available())
			yield();

		if(!input.connected())
		{
#ifdef DEBUG
			Serial.println("Server closed connection");
#endif
			input.stop();
			input.flush();
			return false;
		}

		if(input.read() == '\r' && input.read() == '\n')
		{
			if(startOfLine)
				break;
			else
				startOfLine = true;
		}
	}

	return true;
}

static void startPlaying(const char *url)
{
	if(playing)
		stopPlaying();

#ifdef DEBUG
	Serial.print("Playing ");
	Serial.println(url);
#endif

	if(strncmp(url, "http://", 7) != 0)
	{
#ifdef DEBUG
		Serial.println("Not a valid http url");
#endif
		return;
	}

	const char *host = url + 7;
	const char *path = strchr(host, '/');
	if(path == NULL)
	{
#ifdef DEBUG
		Serial.println("Not a valid http path");
#endif
		return;
	}

	int hostLen = path - host;
	char _host[hostLen + 1];
	memcpy(_host, host, hostLen);
	_host[hostLen] = 0;

	if(path[0] == 0)
		path = "/";

	if(http_open(_host, path))
	{
		output.startSong();
		buffpos = 0;
		bufflen = 0;
		playing = true;
#ifdef DEBUG
		Serial.println("Successfully started playing");
#endif
	}
}
static void stopPlaying()
{
	if(!playing)
		return;

#ifdef DEBUG
	Serial.println("Stopping playback");
#endif

	if(input.connected())
	{
		input.stop();
		input.flush();
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

void mqtt_callback(char *topic, uint8_t *payload, unsigned len)
{
#ifdef DEBUG
	Serial.print("Received MQTT message on ");
	Serial.print(topic);
	Serial.print(" of length ");
	Serial.print(len);
	Serial.print(": ");
	for(unsigned i = 0; i < len; i++)
		Serial.print((char)payload[i]);
	Serial.println();
#endif

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
	}
	else if(strcmp(topic, "/volume") == 0)
	{
		uint8_t oldVolume = volume;

		char *end;
		volume = strntou((char *)payload, len, &end);
		if(end != (char *)payload + len)
			return;

		if(volume != oldVolume)
		{
#ifdef DEBUG
			Serial.print("Set volume to ");
			Serial.println(volume);
#endif
			output.setVolume(volume);
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

#ifdef DEBUG
			Serial.print("Setting tone to ");
			for(int i = 0; i < 4; i++)
			{
				Serial.print(values[i]);

				if(i != 3)
					Serial.print("/");
			}
			Serial.println();
#endif
		output.setTone(values);
	}
	else if(strcmp(topic, "/station") == 0)
	{
		if(len > 1023 || (playing && strncmp(stationUrl, (char *)payload, len) == 0))
			return;

		memcpy(stationUrl, payload, len);
		stationUrl[len] = 0;
		startPlaying(stationUrl);
	}
}

void setup()
{
#ifdef DEBUG
	Serial.begin(115200);
	delay(1000);
	Serial.println();
	Serial.println("Wake up.");
#endif

	WiFi.persistent(false);
	WiFi.mode(WIFI_OFF);
	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PSK);

	mqtt.setServer(MQTT_SERVER, MQTT_PORT);
	mqtt.setCallback(mqtt_callback);

	connections.setup();

	SPI.begin();
	output.begin();
	output.switchToMp3Mode();
	output.setVolume(volume);

	buffer = (uint8_t *)malloc(BUFFER_SIZE);
	buffpos = 0;
	bufflen = 0;
}

void loop()
{
	connections.loop([]()
	{
		mqtt.subscribe(MQTT_TOPIC "/playing");
		mqtt.subscribe(MQTT_TOPIC "/volume");
		mqtt.subscribe(MQTT_TOPIC "/tone");
		mqtt.subscribe(MQTT_TOPIC "/station");
	});

	if(playing)
	{
		if(!input.connected())
		{
#ifdef DEBUG
			Serial.println("Connection to server lost");
#endif
			playing = false;
			startPlaying(stationUrl);

			if(!playing)
				return;
		}

		size_t bytesAvailable = input.available();
		if(bufflen < BUFFER_SIZE && bytesAvailable > 0)
		{
			uint8_t *start = &buffer[(buffpos + bufflen) % BUFFER_SIZE];
			uint8_t *buffend = buffer + BUFFER_SIZE;

			size_t len = bytesAvailable;
			if(start + len >= buffend)
				len = buffend - start;
			if(BUFFER_SIZE - bufflen < len)
				len = BUFFER_SIZE - bufflen;

			input.readBytes(start, len);
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
}
