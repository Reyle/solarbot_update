#pragma once
#include "Socket.h"
#include "ServerSocket.h"
#include "SocketException.h"

class RobotNetClient
{
public:
	volatile bool IsWorking = false;
	void start(int id);
	int send(char* data, int length);
	void stop();

	void* ClientThread();
	pthread_t client_thread;
	
	static void* client_thread_helper(void* context)
	{
		//return ((new_connection_event*)context)->this_pointer->ClientThread(((new_connection_event*)context)->socket_fd);
		return ((RobotNetClient*)context)->ClientThread();
	}

private:
	int m_socket_fd;
};

