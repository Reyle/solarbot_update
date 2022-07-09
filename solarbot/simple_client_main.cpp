#include "ClientSocket.h"
#include "SocketException.h"
#include <iostream>
#include <string>

int main_socket_client(int argc, int argv[])
{
	while (true)
	{
		try
		{
			std::cout << "Connecting...";
			std::cout.flush();
			ClientSocket client_socket("172.16.1.41", 2010);
			std::cout << "OK\n";

			char* text = "Hola mundo cruel\n";

			std::string reply;

			try
			{
				while (1)
				{
					client_socket << "Test message.";
					//client_socket.Write((unsigned char*)text, 17);
					usleep(250000);
				}

				//client_socket >> reply;
			}
			catch (SocketException& e) 
			{
				std::cout << "ERROR:" << e.description() << "\n";
				std::cout.flush();
			}

			//std::cout << "We received this response from the server:\n\"" << reply << "\"\n";;

		}
		catch (SocketException& e)
		{
			std::cout << "Exception was caught:" << e.description() << "\n";
		}

		usleep(500000);
	}

	return 0;
}