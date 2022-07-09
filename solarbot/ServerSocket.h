// Definition of the ServerSocket class

#ifndef ServerSocket_class
#define ServerSocket_class

#include "Socket.h"


class ServerSocket : public Socket
{
public:

	ServerSocket(int port);
	ServerSocket() {};
	virtual ~ServerSocket();

	const ServerSocket& operator << (const std::string&) const;
	const ServerSocket& operator >> (std::string&) const;

	void accept(ServerSocket&);

	bool Write(unsigned char* buff, int length);
	int Read(unsigned char* buff, int size);

	int GetSockectId();
	sockaddr_in GetSockectAddress();
	int Valid();
	void Close();

};


#endif