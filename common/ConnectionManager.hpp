#pragma once

#include "./Log.hpp"
#include "./Blink.hpp"

class ConnectionManager
{
private:
	PubSubClient& mqtt;
	void (*mqttReconnect)();
	bool hadWifiLoss = true;

public:
	uint8_t lastWillQos = MQTTQOS0;
	const char *lastWillTopic = NULL;
	const char *lastWillMessage = NULL;
	bool lastWillRetain = true;

	ConnectionManager(PubSubClient& _mqtt, void (*_mqttReconnect)())
		: mqtt(_mqtt), mqttReconnect(_mqttReconnect)
	{
	}

	bool loop()
	{
		if(hadWifiLoss)
		{
			if(WiFi.status() != WL_CONNECTED)
			{
				blink(1);
				return false;
			}
			else
			{
				LOGLN("");
				LOG("Connected, IP-Address: ");
				LOGLN(WiFi.localIP());

				hadWifiLoss = false;
			}
		}
		else if(WiFi.status() != WL_CONNECTED)
		{
			LOG("(Re-)connecting to WiFi ");
			LOGLN(WIFI_SSID);

			hadWifiLoss = true;
		}
		
		if(mqtt.connected())
		{
			mqtt.loop();
		}
		else
		{
			LOG("(Re-)connecting to MQTT broker at ");
			LOG(MQTT_SERVER);
			LOG(":");
			LOGLN(MQTT_PORT);

			if(mqtt.connect(MQTT_CLIENT_ID, lastWillTopic, lastWillQos, lastWillRetain, lastWillMessage))
			{
				LOGLN("");
				LOGLN("Connected");

				mqttReconnect();
			}
			else
			{
				blink(2);
				return false;
			}
		}

		return true;
	}
};
