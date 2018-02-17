#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "./config.h"

enum
{
	ACTION_OFF,
	ACTION_ON,
	ACTION_TOGGLE,
};

#define GPIO_BUTTON 0
#define GPIO_LED 13
#define GPIO_RELAY 12

bool buttonPressed = false;
uint8_t led = HIGH;
uint8_t relay = LOW;

WiFiClient client;
PubSubClient mqtt(client);

void blink(uint8_t count, uint32_t totalSleep)
{
	for(int i = 0; i < count; i++)
	{
		digitalWrite(GPIO_LED, LOW);
		delay(100);
		digitalWrite(GPIO_LED, HIGH);
		delay(100);
	}

	delay(totalSleep - count * 200);
}

uint8_t performAction(uint8_t old, uint8_t action)
{
	if(old == LOW && action != ACTION_OFF)
		return HIGH;
	else if(old == HIGH && action != ACTION_ON)
		return LOW;
	else
		return old;
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
	if(strncmp(topic, MQTT_TOPIC, prefixLen) == 0)
	{
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
			led = relay;
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
#ifdef DEBUG
			Serial.print("Set relay to ");
			Serial.println(relay);
#endif

			digitalWrite(GPIO_RELAY, relay);

			if(mqtt.connected())
				mqtt.publish(MQTT_TOPIC "/relay", relay == HIGH ? "1" : "0", true);
		}
		if(oldLed != led)
		{
#ifdef DEBUG
			Serial.print("Set led to ");
			Serial.println(led);
#endif

			digitalWrite(GPIO_LED, led);

			if(mqtt.connected())
				mqtt.publish(MQTT_TOPIC "/led", led == HIGH ? "1" : "0", true);
		}
	}
}

void setup()
{
#ifdef DEBUG
	Serial.begin(115200);
	delay(5000);
	Serial.println("");
#endif

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
	if(WiFi.status() != WL_CONNECTED)
	{
#ifdef DEBUG
		Serial.print("(Re-)connecting to WiFi ");
		Serial.println(WIFI_SSID);
#endif

		while (WiFi.status() != WL_CONNECTED)
		{
			blink(1, 1000);
		}
		digitalWrite(GPIO_LED, led);

#ifdef DEBUG
		Serial.print("Connected, ");
		Serial.print("IP-Address: ");
		Serial.println(WiFi.localIP());
#endif
	}

	if(!mqtt.connected())
	{
#ifdef DEBUG
		Serial.print("(Re-)connecting to MQTT broker at ");
		Serial.print(MQTT_SERVER);
		Serial.print(":");
		Serial.println(MQTT_PORT);
#endif

		while(!mqtt.connect(MQTT_CLIENT_ID))
		{
			blink(2, 1000);
		}

#ifdef DEBUG
		Serial.println("Connected");
#endif

		digitalWrite(GPIO_LED, led);

		mqtt.publish(MQTT_TOPIC "/relay", relay == HIGH ? "1" : "0", true);
		mqtt.publish(MQTT_TOPIC "/led", led == HIGH ? "1" : "0", true);
		mqtt.subscribe(MQTT_TOPIC);
		mqtt.subscribe(MQTT_TOPIC "/led");
		mqtt.subscribe(MQTT_TOPIC "/relay");
	}

	mqtt.loop();

	if(digitalRead(GPIO_BUTTON) == 0)
	{
		if(!buttonPressed)
		{
#ifdef DEBUG
			Serial.println("Button press detected. Emulating MQTT receive...");
#endif
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
