// Definition of the Socket class

#ifndef Socket_class
#define Socket_class


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>


const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 5;
const int MAXRECV = 500;

class Socket
{
public:
	Socket();
	virtual ~Socket();

	// Server initialization
	bool create();
	bool bind(const int port);
	bool listen() const;
	bool accept(Socket&) const;
	void close();

	// Client initialization
	bool connect(const std::string host, const int port);

	// Data Transimission
	bool send(const std::string) const;
	bool write(const unsigned char* buff, int length);
	int recv(std::string&) const;
	int read(unsigned char* buff, int size);
	int read_with_status(unsigned char* buff, int size);


	void set_non_blocking(const bool);
	void set_read_timeout(int seconds); //in seconds

	bool is_valid() const { return m_sock != -1; }



	int m_sock;
	sockaddr_in m_addr;

private:

};


#endif