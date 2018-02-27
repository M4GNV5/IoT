#pragma once

class ConnectionManager
{
public:
	PubSubClient& mqtt;
	ConnectionManager(PubSubClient& _mqtt) : mqtt(_mqtt)
	{
	}

	void blink(uint8_t count, uint32_t totalSleep)
	{
		/* TODO re-add blinking
		for(int i = 0; i < count; i++)
		{
			digitalWrite(GPIO_LED, LOW);
			delay(100);
			digitalWrite(GPIO_LED, HIGH);
			delay(100);
		}*/

#ifdef DEBUG
		Serial.print(".");
#endif

		//delay(totalSleep - count * 200);
		delay(totalSleep);
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

			while(!mqtt.connect(MQTT_CLIENT_ID))
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
