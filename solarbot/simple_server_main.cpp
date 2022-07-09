#include "ServerSocket.h"
#include "SocketException.h"
#include <string>
#include <iostream>


volatile int client_phread_index = 0;

void* clientThread(void* arg)
{
	try
	{
		//ServerSocket* p = (ServerSocket*)arg;// *((ServerSocket*)arg);

		//ServerSocket client = *p;

		//Socket client = *((Socket*)arg);

		//int newSocket = *((int*)arg);
		Socket client;
		client.m_sock = *((int*)arg);


		client.send("Hola Mundo Cruel\n");
		sleep(1);
		client.~Socket();		
	}
	catch (SocketException& e) 
	{
		std::cout << "\nException on Thread was caught:" << e.description() << "\n";
	}

	client_phread_index--;
	pthread_exit(NULL);
}

int main_k3(int argc, int argv[])
{
	std::cout << "running....\n";

	pthread_t clients_threads[MAXCONNECTIONS];
	client_phread_index = 0;
	while (1)
	{
		try
		{
			// Create the socket
			Socket server;
			if (!server.create())
			{
				throw SocketException("Could not create server socket.");
			}

			if (!server.bind(2010))
			{
				throw SocketException("Could not bind to port.");
			}

			if (!server.listen())
			{
				throw SocketException("Could not listen to socket.");
			}



			while (true)
			{

				Socket new_sock;
				server.accept(new_sock);


				//try
				//{
				//	new_sock << "Merda OK?\n";
				//}
				//catch (SocketException& ee) 
				//{
				//	std::cout << "Merda: " << ee.description() << ".\n";
				//}
				//sleep(1);
				//new_sock.~ServerSocket();



				//try
				//{
				//	while (true)
				//	{
				//		std::string data;
				//		new_sock >> data;
				//		new_sock << data;
				//	}
				//}
				//catch (SocketException&) {}



				//for each client request creates a thread and assign the client request to it to process
				//so the main thread can entertain next request

				if (client_phread_index < MAXCONNECTIONS)
				{
					if (pthread_create(&clients_threads[client_phread_index], NULL, clientThread, &new_sock.m_sock) != 0)
						printf("Failed to create thread\n");
					else
						client_phread_index++;
				}
				else
				{
					try
					{
						while (true)
						{
							new_sock.send("Too many connections. Exiting...\n");
							new_sock.~Socket();
						}
					}
					catch (SocketException&) {}
				}



				//if (i >= MAXCONNECTIONS)
				//{
				//	i = 0;
				//	while (i < MAXCONNECTIONS)
				//	{
				//		pthread_join(clients_threads[i++], NULL);
				//	}
				//	i = 0;
				//}

			}
		}
		catch (SocketException& e)
		{
			std::cout << "Exception was caught:" << e.description() << "\nExiting.\n";
		}

		usleep(500000);
	}


	//pthread_t tid[60];
	//int i = 0;
	//while (1)
	//{
	//	/*---- Accept call creates a new socket for the incoming connection ----*/
	//	addr_size = sizeof serverStorage;
	//	newSocket = accept(serverSocket, (struct sockaddr*) & serverStorage, &addr_size);

	//	//for each client request creates a thread and assign the client request to it to process
	//   //so the main thread can entertain next request
	//	if (pthread_create(&tid[i], NULL, socketThread, &newSocket) != 0)
	//		printf("Failed to create thread\n");
	//	if (i >= 5)
	//	{
	//		i = 0;
	//		while (i < 5)
	//		{
	//			pthread_join(tid[i++], NULL);
	//		}
	//		i = 0;
	//	}
	//}

	return 0;
}