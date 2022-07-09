#include <iostream>
#include "RobotNetServer.h"

int RobotNetServer::start(uint16_t port)
{
	//int result = 0;

	//try
	//{
	//	// Create the socket		
	//	if (!server.create())
	//	{
	//		throw SocketException("Could not create server socket.");
	//	}

	//	if (!server.bind(port))
	//	{
	//		throw SocketException("Could not bind to port.");
	//	}

	//	if (!server.listen())
	//	{
	//		throw SocketException("Could not listen to socket.");
	//	}

	//	pthread_create(&server_thread, NULL, &RobotNetServer::server_thread_helper, this);
	//}
	//catch (SocketException & e)
	//{
	//	std::cout << "Exception was caught:" << e.description() << "\nExiting.\n";
	//	result = -1;
	//}

	//return result;

	for (int i = 0; i < MAXCONNECTIONS; i++)
		clients[i] == NULL;

	m_port = port;
	server_working = true;
	return pthread_create(&server_thread, NULL, &RobotNetServer::server_thread_helper, this);

}

void* RobotNetServer::ServerThread()
{
	ServerSocket client_socket;
	ServerSocket server_socket(m_port);

	while (server_working)
	{
		////gps->getLocation(&location); //Get GPS position
		//cout << "tick" << endl;
		//usleep(500000);
		server_socket.accept(client_socket);

		if (clients_count < MAXCONNECTIONS)
		{
			////Look for the available socket for client
			//int i = 0;
			//for (i = 0; i < MAXCONNECTIONS; i++)
			//{
			//	if (clients[i]==NULL || !clients[i]->IsWorking)
			//		break;
			//}

			//if (i < MAXCONNECTIONS)
			//{
			//	int id = client_socket.GetSockectId();

			//	if (clients[i] == NULL)
			//		clients[i] = new RobotNetClient();

			//	clients[i]->start(id);
			//}
			//else
			//{
			//	client_socket.Close();
			//	cout << "No available socket for this new connection" << endl;
			//}

			struct new_connection_event event;
			event.this_pointer = this;
			int id = client_socket.GetSockectId();
			event.fd = &id;

			if (pthread_create(&threads[clients_count], NULL, client_thread_helper, &event) != 0)
			{
				cout << "Failed to create thread" << endl;
			}
			else
			{
				clients_count++;
			}
		}
	}
	
	return nullptr;
}

void* RobotNetServer::handle_client(int* fd)
{
	try
	{
		//ServerSocket* p = (ServerSocket*)arg;// *((ServerSocket*)arg);

		//ServerSocket client = *p;

		//Socket client = *((Socket*)arg);

		//int newSocket = *((int*)arg);
		Socket client;
		//client.m_sock = *((int*)fd);
		client.m_sock = *fd;

		while (client.is_valid())
		{
			client.send("Hola Mundo Cruel\n");
			sleep(1);
		}
		client.~Socket();
		std::cout << "\nChao" << "\n";

	}
	catch (SocketException & e)
	{
		std::cout << "\nException on Thread was caught:" << e.description() << "\n";
	}
		
	clients_count--;
	
	return nullptr;
}
