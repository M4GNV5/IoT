#pragma once

#include "./Log.hpp"

#ifndef HTTP_USER_AGENT
#define HTTP_USER_AGENT "IoT Device - https://github.com/M4GNV5/IoT"
#endif

#ifndef HTTP_HEADER_BUFF_LEN
#define HTTP_HEADER_BUFF_LEN 256
#endif

//very simplistic HTTP get
bool http_get(WiFiClient *sock, const char *host, uint16_t port, const char *path,
	void (*header_handler)(char *, bool))
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

	bool startOfLine = false;
	size_t i = 0;
	char buff[HTTP_HEADER_BUFF_LEN];
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

		char curr = sock->read();
		
		if(curr == '\r' && sock->peek() == '\n')
		{
			sock->read(); //skip '\n'

			if(startOfLine)
				break;

			if(header_handler != NULL)
			{
				buff[i] = 0;
				header_handler(buff, i < HTTP_HEADER_BUFF_LEN);
			}

			startOfLine = true;
			i = 0;
		}
		else if(header_handler != NULL && i < HTTP_HEADER_BUFF_LEN - 1)
		{
			startOfLine = false;
			buff[i] = curr;
			i++;
		}
	}

	return true;
}

bool http_get(WiFiClient *sock, const char *host, uint16_t port, const char *path)
{
	return http_get(sock, host, port, path);
}