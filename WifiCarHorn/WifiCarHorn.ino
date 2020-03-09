#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#ifndef APSSID
#define APSSID "TukTuk"
#define APPSK  "ESP8266_TukTuk"
#endif

const char relON[] = { 0xA0 , 0x01 , 0x01 , 0xA2 };
const char relOFF[] = { 0xA0 , 0x01 , 0x00 , 0xA1 };
const char relPrefix[] = "\n+IPD,0,4:";

ESP8266WebServer server(80);
bool isRelayOpen = false;

void handleIndex()
{
	server.send(200, "text/html", "<html charset=\"utf-8\">\n<head>\n\t<title>\n\t\tTwizy Tuuut Tuuut!\n\t</title>\n\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n\t<style>\n\t\tbutton {\n\t\t\twidth: 100%;\n\t\t\tmargin-bottom: 10px;\n\t\t\theight: 55px;\n\t\t\tfont-size: 15px;\n\t\t\tborder-radius: 12px;\n\t\t\tbackground-color: black;\n\t\t}\n\n\t\t#btn-timed {\n\t\t\tcolor: yellow;\n\t\t}\n\n\t\t#btn-enable {\n\t\t\tcolor: green;\n\t\t}\n\n\t\t#btn-disable {\n\t\t\tcolor: red;\n\t\t}\n\n\t\tbody {\n\t\t\tcolor: white;\n\t\t\tbackground-color: black;\n\t\t}\n\n\t\tp {\n\t\t\ttext-align: center;\n\t\t\twidth: 100%;\n\t\t}\n\n\t\t#status {\n\t\t\tfont-style: italic;\n\t\t}\n\t</style>\n\t<script>\n\t\tvar currentlyActive = false;\n\t\tfunction checkStatus() {\n\t\t\tfetch(\"/status\")\n\t\t\t\t.then(response => response.text())\n\t\t\t\t.then(data => {\n\t\t\t\t\tconsole.log(data);\n\t\t\t\t\tdata = data === \"true\";\n\t\t\t\t\tdocument.getElementById(\"status\").innerText = data ? \"Tuuut Tuuut!\" : \"Aus\";\n\t\t\t\t});\n\t\t}\n\t\tfunction ajax(to) {\n\t\t\treturn fetch(\"/set/\" + to)\n\t\t\t\t.then(() => checkStatus());\n\t\t}\n\t\tfunction enableOnce() {\n\t\t\tif(currentlyActive)\n\t\t\t\treturn;\n\t\t\tcurrentlyActive = true;\n\n\t\t\tajax(\"on\");\n\t\t\tsetTimeout(() => {\n\t\t\t\tajax(\"off\")\n\t\t\t\tcurrentlyActive = false;\n\t\t\t}, 2000);\n\t\t}\n\t</script>\n</head>\n\t<body onload=\"checkStatus();\">\n\t\t<p>Aktueller Status: <span id=\"status\">Unbekannt</span></p>\n\t\t<button id=\"btn-timed\" onclick=\"enableOnce();\">Tuuut Tuuut!</button><br>\n\t\t<button id=\"btn-enable\" onclick=\"ajax('on');\">Aktivieren</button><br>\n\t\t<button id=\"btn-disable\" onclick=\"ajax('off');\">Deaktivieren</button><br>\n\t</body>\n</html>");
}

void handleSetOn()
{
	Serial.write(relPrefix, sizeof(relPrefix));
	Serial.write(relON, sizeof(relON));
	isRelayOpen = true;

	server.send(200, "text/html", "ok cool");
}

void handleSetOff()
{
	Serial.write(relPrefix, sizeof(relPrefix));
	Serial.write(relOFF, sizeof(relOFF));
	isRelayOpen = false;

	server.send(200, "text/html", "ok cool");
}

void handleGetStatus()
{
	server.send(200, "text/html", isRelayOpen ? "true" : "false");
}

void setup()
{
	delay(2000);
	
	WiFi.mode(WIFI_OFF);
	delay(500);
	WiFi.softAP(APSSID, APPSK);

	server.on("/", handleIndex);
	server.on("/set/on", handleSetOn);
	server.on("/set/off", handleSetOff);
	server.on("/status", handleGetStatus);
	server.begin();

	Serial.begin(115200);

	Serial.write("\r\nWIFI CONNECTED\nWIFI GOT IP\n");
}

void loop()
{
	server.handleClient();
}
