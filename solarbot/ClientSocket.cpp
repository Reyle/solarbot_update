// Implementation of the ClientSocket class

#include "ClientSocket.h"
#include "SocketException.h"


ClientSocket::ClientSocket(std::string host, int port)
{
	if (!Socket::create())
	{
		throw SocketException("Could not create client socket.");
	}

	if (!Socket::connect(host, port))
	{
		throw SocketException("Could not bind to port.");
	}

}


const ClientSocket& ClientSocket::operator << (const std::string& s) const
{
	if (!Socket::send(s))
	{
		throw SocketException("Could not write to socket.");
	}

	return *this;

}


const ClientSocket& ClientSocket::operator >> (std::string& s) const
{
	if (!Socket::recv(s))
	{
		throw SocketException("Could not read from socket.");
	}

	return *this;
}

bool ClientSocket::Write(unsigned char* buff, int length)
{
	if (!Socket::write(buff, length))
	{
		throw SocketException("Could not write to socket.");
		return false;
	}

	return true;
}

int ClientSocket::Read(unsigned char* buff, int size)
{
	int r = Socket::read(buff, size);
	if (r > 0)
	{
		return r;
	}
	else
	{
		throw SocketException("Could not read from socket.");
		return 0;
	}
}
