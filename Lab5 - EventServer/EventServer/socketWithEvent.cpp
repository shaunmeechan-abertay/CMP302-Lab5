#include "socketWithEvent.h"

socketWithEvent::socketWithEvent(std::string newName)
{
	name = newName;
}

SOCKET socketWithEvent::getSocket()
{
	return socket;
}

WSAEVENT socketWithEvent::getEvent()
{
	return event;
}

std::string socketWithEvent::getName()
{
	return name;
}

void socketWithEvent::setSocket(SOCKET newSocket)
{
	 socket = newSocket;
	 return;
}

void socketWithEvent::setEvent(WSAEVENT newEvent)
{
	 event = newEvent;
	 return;
}

void socketWithEvent::setName(std::string newName)
{
	 name = newName;
	 return;
}
