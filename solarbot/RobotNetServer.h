

#include "Socket.h"
#include "ServerSocket.h"
#include "SocketException.h"
#include <vector>
#include <thread>
#include "RobotNetClient.h"

using namespace std;

class RobotNetServer
{
public:
	static const int MAXCONNECTIONS = 3;
	int start(uint16_t port);

	



private:
	//ServerSocket server;
	void* ServerThread();
	int m_port;
	

	volatile unsigned char server_working;
	pthread_t server_thread;	
	static void* server_thread_helper(void* context)
	{
		return ((RobotNetServer*)context)->ServerThread();
	}

	struct new_connection_event
	{
		RobotNetServer* this_pointer;
		int* fd;
	};

	pthread_t threads[MAXCONNECTIONS];
	RobotNetClient* clients[MAXCONNECTIONS];
	void* handle_client(int* fd);
	static void* client_thread_helper(void* context)
	{
		return ((new_connection_event*)context)->this_pointer->handle_client(((new_connection_event*)context)->fd);
		//return ((new_connection_event*)context)->this_pointer->handle_client(((new_connection_event*)context)->fd);
	}
	

	volatile int clients_count = 0;
};