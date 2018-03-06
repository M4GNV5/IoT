#pragma once

class ConnectionManager
{
private:
	PubSubClient& mqtt;
	uint8_t ledPin;
	uint8_t ledOff;

public:
	uint8_t lastWillQos = MQTTQOS0;
	const char *lastWillTopic = NULL;
	const char *lastWillMessage = NULL;
	bool lastWillRetain = true;

	ConnectionManager(PubSubClient& _mqtt, uint8_t _ledPin, uint8_t _ledOff)
		: mqtt(_mqtt), ledPin(_ledPin), ledOff(_ledOff)
	{
	}

	void blink(uint8_t count, uint32_t totalSleep)
	{
		for(int i = 0; i < count; i++)
		{
			digitalWrite(ledPin, !ledOff);
			delay(100);
			digitalWrite(ledPin, ledOff);
			delay(100);
		}

#ifdef DEBUG
		Serial.print(".");
#endif

		delay(totalSleep - count * 200);
	}

	void setup()
	{
		pinMode(ledPin, OUTPUT);
		digitalWrite(ledPin, ledOff);
	}

	void loop(void (*mqtt_reconnect)())
	{
		if(WiFi.status() != WL_CONNECTED)
		{
#ifdef DEBUG
			Serial.print("(Re-)connecting to WiFi ");
			Serial.print(WIFI_SSID);
#endif

			while (WiFi.status() != WL_CONNECTED)
			{
				blink(1, 1000);
			}

#ifdef DEBUG
			Serial.println();
			Serial.print("Connected, IP-Address: ");
			Serial.println(WiFi.localIP());
#endif
		}

		if(!mqtt.connected())
		{
#ifdef DEBUG
			Serial.print("(Re-)connecting to MQTT broker at ");
			Serial.print(MQTT_SERVER);
			Serial.print(":");
			Serial.print(MQTT_PORT);
#endif

			while(!mqtt.connect(MQTT_CLIENT_ID, lastWillTopic, lastWillQos, lastWillRetain, lastWillMessage))
			{
				blink(2, 1000);
			}

#ifdef DEBUG
			Serial.println();
			Serial.println("Connected");
#endif

			mqtt_reconnect();
		}

		mqtt.loop();
	}
};
