// EventServer.cpp : Event-based server example - By Gaz Robinson 2020
//

#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <list>

#include "NetUtility.h"
#include "socketWithEvent.h"

#define SERVERIP "127.0.0.1"
#define SERVERPORT 5555
#define MESSAGESIZE 40

//TODO: Boy, having a socket variable for each client will get tiring fast.
// It would be nice to have a nicer system for this...
socketWithEvent ListenSocket("Listen Socket");
socketWithEvent AcceptSocket("Accept Socket");
int eventCount = 2;
int clientCount = 0;

//Structure to hold the result from WSAEnumNetworkEvents
WSANETWORKEVENTS NetworkEvents;
WSAEVENT AcceptEvent;

//Prototypes
void CleanupSocket();

int main()
{
	StartWinSock();

	printf("Server starting\n");

	//Build socket address structure for binding the socket
	sockaddr_in InetAddr;
	InetAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &(InetAddr.sin_addr));
	InetAddr.sin_port = htons(SERVERPORT);

	//Create our TCP server/listen socket
	ListenSocket.setSocket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
	if (ListenSocket.getSocket() == INVALID_SOCKET) {
		die("socket failed");
	}

	// Bind the server socket to its address.
	if (bind(ListenSocket.getSocket(), (SOCKADDR*)&InetAddr, sizeof(InetAddr)) != 0) {
		die("bind failed");
	}

	// Create a new event for checking listen socket activity
	ListenSocket.setEvent(WSACreateEvent());
	if (ListenSocket.getEvent() == WSA_INVALID_EVENT) {
		die("server event creation failed");
	}
	//Assosciate this event with the socket types we're interested in
	//In this case, on the server, we're interested in Accepts and Closes
	//WSAEventSelect(ListenSocket.getSocket(), ListenEvent, FD_ACCEPT | FD_CLOSE);
	WSAEventSelect(ListenSocket.getSocket(), ListenSocket.getEvent(), FD_ACCEPT | FD_CLOSE);

	//Start listening for connection requests on the socket
	if (listen(ListenSocket.getSocket(), 1) == SOCKET_ERROR) {
		die("listen failed");
	}

	eventCount++;

	printf("Listening on socket...\n");

	//Which event has activity?
	int eventIndex = 0;
	
	//Create an event to use as a pointer for WSAWaitForMultipleEvents	
	WSAEVENT ListenEvent = ListenSocket.getEvent();

	// The list of clients currently connected to the server
	std::list<socketWithEvent*> conns;

	while (true) {
		// The structure that describes the set of sockets we're interested in.
		fd_set readable;
		FD_ZERO(&readable);

		// The structure of writable sockets
		fd_set writable;
		FD_ZERO(&writable);

		// Add the server socket, which will become "readable" and "writable" if there's a new
		// connection to accept.
		FD_SET(ListenSocket.getSocket(), &readable);
		FD_SET(ListenSocket.getSocket(), &writable);

		// Add all of the connected clients' sockets.
		for (auto conn : conns)
		{
			if (conn->wantRead())
			{
				FD_SET(conn->getSocket(), &readable);
			}

			if (conn->wantWrite())
			{
				FD_SET(conn->getSocket(), &writable);
			}
		}

		//Check our events for activity. 
		//Params: How many events, pointer to an array of events, should we wait on ALL events being ready?, how long should we wait?, ignore this one for now...
		//At the moment we're using a timeout of 0 to 'poll' for activity, we could move this to a seperate thread and let it block there to make it more efficient!
		//Here we check for events on the ListenEvent		
		DWORD returnVal;

		if (FD_ISSET(ListenSocket.getSocket(),&readable))
		{
			if (clientCount < 2) {
				returnVal = WSAWaitForMultipleEvents(1, &ListenEvent, false, 0, false);

				if ((returnVal != WSA_WAIT_TIMEOUT) && (returnVal != WSA_WAIT_FAILED)) {
					eventIndex = returnVal - WSA_WAIT_EVENT_0; //In practice, eventIndex will equal returnVal, but this is here for compatability

					if (WSAEnumNetworkEvents(ListenSocket.getSocket(), ListenEvent, &NetworkEvents) == SOCKET_ERROR) {
						die("Retrieving event information failed");
					}
					if (NetworkEvents.lNetworkEvents & FD_ACCEPT)
					{
						if (NetworkEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
							printf("FD_ACCEPT failed with error %d\n", NetworkEvents.iErrorCode[FD_ACCEPT_BIT]);
							break;
						}
						// Accept a new connection, and add it to the socket and event lists
						sockaddr_in clientAddr;
						int addrSize = sizeof(clientAddr);
						socketWithEvent clientSocket("Client Socket");
						clientSocket.setSocket(accept(ListenSocket.getSocket(), (sockaddr*)&clientAddr, &addrSize));
						if (clientSocket.getSocket() == INVALID_SOCKET)
						{
							printf("ERROR: Accept failed\n");
							continue;
						}
						clientSocket.setEvent(WSACreateEvent());
						//AcceptSocket.setSocket(accept(ListenSocket.getSocket(), NULL, NULL));
						//AcceptSocket.setEvent(WSACreateEvent);
						if (clientSocket.getEvent() == WSA_INVALID_EVENT)
						{
							die("ERROR: Invalid event created!");
						}
						//TODO: It'd be great if we could wait for a Read or Write event too...
						WSAEventSelect(clientSocket.getSocket(), clientSocket.getEvent(), FD_CLOSE);

						conns.push_back(new socketWithEvent(clientSocket));
						clientCount++;

						printf("Socket %d connected\n", clientSocket.getSocket());
					}
				}
				else if (returnVal == WSA_WAIT_TIMEOUT) {
					//All good, we just have no activity
				}
				else if (returnVal == WSA_WAIT_FAILED) {
					die("WSAWaitForMultipleEvents failed!");
				}
			}
		}
		if (clientCount > 0) {
			for (auto it = conns.begin(); it != conns.end();)
			{
				socketWithEvent* conn = *it;
				AcceptEvent = conn->getEvent();
				returnVal = WSAWaitForMultipleEvents(1, &AcceptEvent, false, 0, false);
				if ((returnVal != WSA_WAIT_TIMEOUT) && (returnVal != WSA_WAIT_FAILED)) {
					eventIndex = returnVal - WSA_WAIT_EVENT_0; //In practice, eventIndex will equal returnVal, but this is here for compatability

					if (WSAEnumNetworkEvents(AcceptSocket.getSocket(), AcceptSocket.getEvent(), &NetworkEvents) == SOCKET_ERROR) {
						die("Retrieving event information failed");
					}
					if (NetworkEvents.lNetworkEvents & FD_CLOSE)
					{
						//We ignore the error if the client just force quit
						if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0 && NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 10053)
						{
							printf("FD_CLOSE failed with error %d\n", NetworkEvents.iErrorCode[FD_CLOSE_BIT]);
							break;
						}
						CleanupSocket();
					}
				}
				else if (returnVal == WSA_WAIT_TIMEOUT) {
					//All good, we just have no activity
				}
				else if (returnVal == WSA_WAIT_FAILED) {
					die("WSAWaitForMultipleEvents failed!");
				}
			}
		}

		for (auto it = conns.begin(); it != conns.end();)
		{
			socketWithEvent* conn = *it;
			bool dead = false;

			//Is there data to read from this client's socket
			if (FD_ISSET(conn->getSocket(),&readable))
			{
				dead |= conn->doRead();
			}
			if (dead)
			{
				// The client said it was dead -- so free the object,
				// and remove it from the conns list.
				delete conn;
				it = conns.erase(it);
			}
			else
			{
				++it;
			}
		}

	}
}

void CleanupSocket() {

	if (closesocket(AcceptSocket.getSocket()) != SOCKET_ERROR) {
		printf("Successfully closed socket %d\n", AcceptSocket.getSocket());
	}
	if (WSACloseEvent(AcceptSocket.getEvent()) == false) {
		die("WSACloseEvent() failed");
	}
	clientCount = 0;
}