#include "RobotNetClient.h"
#include <iostream>

void RobotNetClient::start(int id)
{
	m_socket_fd = id;
	pthread_create(&client_thread, NULL, &RobotNetClient::client_thread_helper, this);
}

int RobotNetClient::send(char* data, int length)
{
	return 0;
}

void RobotNetClient::stop()
{
}

void* RobotNetClient::ClientThread()
{
	Socket client;
	try
	{
		//ServerSocket* p = (ServerSocket*)arg;// *((ServerSocket*)arg);

		//ServerSocket client = *p;

		//Socket client = *((Socket*)arg);

		//int newSocket = *((int*)arg);
		
		client.m_sock = m_socket_fd;
		IsWorking = true;

		while (IsWorking)
		{
			client.send("Hola Mundo Cruel\n");
			sleep(1);
			if (!client.is_valid())
				IsWorking = false;
		}
	}
	catch (SocketException & e)
	{
		std::cout << "\nException on Client was caught:" << e.description() << "\n";
	}

	client.~Socket();
	IsWorking = false;
	
	return nullptr;
}
