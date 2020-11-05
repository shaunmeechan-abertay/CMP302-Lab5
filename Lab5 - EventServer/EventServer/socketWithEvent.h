#pragma once
#include <winsock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

//The fixed size of the message that we send between the two programs
#define MESSAGESIZE 40

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
	// Return whether this connection is in a state where we want to try
	// reading from the socket.
	bool wantRead();

	// Call this when the socket is ready to read.
	bool doRead();

	//Return whether this connection is in a state where we want to try
	// WRITING to the socket
	bool wantWrite();

	//Call this when the socket is ready to write.
	bool doWrite(std::string message);

private:
	std::string name;
	SOCKET socket;
	WSAEVENT event;
	int readCount_;
	char readBuffer_[MESSAGESIZE];
};

