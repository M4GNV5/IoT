#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "config.h"

#define DEBUG_LED_PIN 13
#define DEBUG_LED_ON HIGH
#include "common/Blink.hpp"

#include "common/Log.hpp"
#include "common/ConnectionManager.hpp"

#define GPIO_BUTTON 0
#define GPIO_RELAY 12
#define GPIO_LED 13

#define tostring(x) (x == HIGH ? "1" : "0")

enum
{
	ACTION_OFF,
	ACTION_ON,
	ACTION_TOGGLE,
};

bool buttonPressed = false;
uint8_t led = HIGH;
uint8_t relay = LOW;

WiFiClient client;
PubSubClient mqtt(client);

void mqtt_reconnect();
ConnectionManager connections(mqtt, mqtt_reconnect);

uint8_t performAction(uint8_t old, uint8_t action)
{
	if(old == LOW && action != ACTION_OFF)
		return HIGH;
	else if(old == HIGH && action != ACTION_ON)
		return LOW;
	else
		return old;
}

void mqtt_reconnect()
{
	mqtt.subscribe(MQTT_TOPIC);
	mqtt.subscribe(MQTT_TOPIC "/led");
	mqtt.subscribe(MQTT_TOPIC "/relay");
}

void mqtt_callback(char *topic, uint8_t *payload, unsigned len)
{
	static int ignoreCount = 0;
	if(ignoreCount > 0)
	{
		ignoreCount--;
		return;
	}

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

	uint8_t action;
	if(len == 1 && payload[0] == '1')
		action = ACTION_ON;
	else if(len == 1 && payload[0] == '0')
		action = ACTION_OFF;
	else if(len == 6 && strncmp((char *)payload, "toggle", 6) == 0)
		action = ACTION_TOGGLE;
	else
		return;

	uint8_t oldLed = led;
	uint8_t oldRelay = relay;

	topic = topic + prefixLen;
	if(topic[0] == 0)
	{
		relay = performAction(relay, action);
		led = !relay;
	}
	else if(strcmp(topic, "/led") == 0)
	{
		led = performAction(led, action);
	}
	else if(strcmp(topic, "/relay") == 0)
	{
		relay = performAction(relay, action);
	}

	if(oldRelay != relay)
	{
		LOG("Set relay to ");
		LOGLN(relay);

		digitalWrite(GPIO_RELAY, relay);

		mqtt.publish(MQTT_TOPIC "/relay", tostring(relay), true);
		ignoreCount++;
	}
	if(oldLed != led)
	{
		LOG("Set led to ");
		LOGLN(led);

		//the led is on when the pin is off, so we need to invert `led` here
		digitalWrite(GPIO_LED, !led);

		mqtt.publish(MQTT_TOPIC "/led", tostring(led), true);
		ignoreCount++;
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
	mqtt.setCallback(mqtt_callback);

	pinMode(GPIO_LED, OUTPUT);
	digitalWrite(GPIO_LED, LOW);

	pinMode(GPIO_RELAY, OUTPUT);
	digitalWrite(GPIO_RELAY, LOW);
}

void loop()
{
	connections.loop();

	if(digitalRead(GPIO_BUTTON) == 0)
	{
		if(!buttonPressed)
		{
			LOGLN("Button press detected. Emulating MQTT receive...");

			mqtt_callback((char *)MQTT_TOPIC, (uint8_t *)"toggle", 6);
			buttonPressed = true;
		}
	}
	else
	{
		buttonPressed = false;
	}

	delay(50);
}
