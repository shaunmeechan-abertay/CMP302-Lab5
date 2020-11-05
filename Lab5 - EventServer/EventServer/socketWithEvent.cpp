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

bool socketWithEvent::wantRead()
{
	//At present, we always do
	return true;
}

bool socketWithEvent::doRead()
{
	// Receive as much data from the client as will fit in the buffer.
	int spaceLeft = (sizeof readBuffer_) - readCount_;
	int count = recv(socket, readBuffer_ + readCount_, spaceLeft, 0);
	if (count <= 0) {
		printf("Client connection closed or broken\n");
		return true;
	}

	// We've successfully read some more data into the buffer.
	readCount_ += count;

	if (readCount_ < MESSAGESIZE) {
		// ... but we've not received a complete message yet.
		// So we can't do anything until we receive some more.
		return false;
	}

	// We've got a complete message.
	printf("Received message from the client: '");
	fwrite(readBuffer_, 1, MESSAGESIZE, stdout);
	printf("'\n");

	if (memcmp(readBuffer_, "quit", 4) == 0) {
		printf("Client asked to quit\n");
		return true;
	}

	// Send the same data back to the client.
	// FIXME: the socket might not be ready to send yet -- so this could block!
	// FIXME: and we might not be able to write the entire message in one go...
	count = send(socket, readBuffer_, MESSAGESIZE, 0);
	if (count != MESSAGESIZE)
	{
		printf("send failed\n");
		return true;
	}

	// Clear the buffer, ready for the next message.
	readCount_ = 0;

	return false;
}

bool socketWithEvent::wantWrite()
{
	//At present we always do
	return true;
}

bool socketWithEvent::doWrite(std::string message)
{
	if (message.length() > MESSAGESIZE)
	{
		printf("ERROR: Unable to send message. Size was larger than MESSAGESIZE.");
		return false;
	}
	else
	{
		//Create the buffer
		char buffer[MESSAGESIZE];
		//Fill the buffer
		memset(buffer, '-', MESSAGESIZE);
		memcpy(buffer, message.c_str(), min(message.size(), MESSAGESIZE));

		if (send(socket, buffer, MESSAGESIZE, 0) != MESSAGESIZE)
		{
			printf("ERROR: Failed to send message");
		}
	}
	return false;
}
