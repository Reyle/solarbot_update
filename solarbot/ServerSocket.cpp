// Implementation of the ServerSocket class

#include "ServerSocket.h"
#include "SocketException.h"


ServerSocket::ServerSocket(int port)
{
	if (!Socket::create())
	{
		throw SocketException("Could not create server socket.");
	}

	if (!Socket::bind(port))
	{
		throw SocketException("Could not bind to port.");
	}

	if (!Socket::listen())
	{
		throw SocketException("Could not listen to socket.");
	}

}

ServerSocket::~ServerSocket()
{
}


const ServerSocket& ServerSocket::operator << (const std::string& s) const
{
	if (!Socket::send(s))
	{
		throw SocketException("Could not write to socket.");
	}

	return *this;

}


const ServerSocket& ServerSocket::operator >> (std::string& s) const
{
	if (!Socket::recv(s))
	{
		throw SocketException("Could not read from socket.");
	}

	return *this;
}

void ServerSocket::accept(ServerSocket& sock)
{
	if (!Socket::accept(sock))
	{
		throw SocketException("Could not accept socket.");
	}
}

bool ServerSocket::Write(unsigned char* buff, int length)
{
	if (!Socket::write(buff, length))
	{
		throw SocketException("Could not write to socket.");
		return false;
	}

	return true;
}

int ServerSocket::Read(unsigned char* buff, int size)
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

int ServerSocket::GetSockectId()
{
	return m_sock;
}

sockaddr_in ServerSocket::GetSockectAddress()
{
	return m_addr;
}

int ServerSocket::Valid()
{
	return is_valid();
}

void ServerSocket::Close()
{
	Socket::close();
}
