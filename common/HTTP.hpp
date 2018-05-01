#pragma once

#include "./Log.hpp"

#ifndef HTTP_USER_AGENT
#define HTTP_USER_AGENT "IoT Device - https://github.com/M4GNV5/IoT"
#endif

//very simplistic HTTP get
bool http_get(WiFiClient *sock, const char *host, uint16_t port, const char *path)
{
	if(!sock->connect(host, port))
	{
		LOG("Could not connect to ");
		LOG(host);
		LOG(":");
		LOGLN(port);
		return false;
	}

	sock->print("GET ");
	sock->print(path);
	sock->println(" HTTP/1.1");

	sock->print("Host: ");
	sock->println(host);

	sock->println("User-Agent: ");
	sock->println("Accept: */*");
	sock->println();

	const char *httpOk = "HTTP/??? 200 OK\r\n";
	const int httpOkLen = strlen(httpOk);
	for(int i = 0; i < httpOkLen; i++)
	{
		while(sock->connected() && !sock->available())
			yield();

		if(!sock->connected() || (sock->read() != httpOk[i] && httpOk[i] != '?'))
		{
			sock->stop();
			sock->flush();
			LOGLN("Unexpected HTTP response");
			return false;
		}
	}

	bool startOfLine = true;
	for(;;)
	{
		while(sock->connected() && !sock->available())
			yield();

		if(!sock->connected())
		{
			sock->stop();
			sock->flush();
			LOGLN("Unexpected HTTP response");
			return false;
		}

		if(sock->read() == '\r' && sock->read() == '\n')
		{
			if(startOfLine)
				break;
			else
				startOfLine = true;
		}
	}

	return true;
}