#pragma once
#include <winsock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

class socketWithEvent
{
public:
	socketWithEvent(std::string name);
	SOCKET getSocket();
	WSAEVENT getEvent();
	std::string getName();
	void setSocket(SOCKET newSocket);
	void setEvent(WSAEVENT newEvent);
	void setName(std::string newName);
private:
	std::string name;
	SOCKET socket;
	WSAEVENT event;
};

