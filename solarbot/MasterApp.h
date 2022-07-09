#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>


#include "Solarbot.h"
#include "AES128.h"
#include "Frame.h"



class MasterApp
{
public:
	MasterApp(uint16_t port, Solarbot* solarbot, volatile int* ctrl);

	std::string SetVisualMode();
	std::string SetWaypointsMode(std::vector<Navigator::Waypoint>);
	std::string SetLineMode();

	void Close();

private:
	const int MAXCONNECTIONS = 5;
	Solarbot* solarbot;
	volatile int* ctrl;
	uint16_t port;

	
	void* ThreadServer();
	int server_socket_fd;
	pthread_t server_thread; //thread to listen for connection
	static void* server_thread_helper(void* context)
	{
		return ((MasterApp*)context)->ThreadServer();
	}
	const static int Server_Rx_Buffer_Size = 512;
	unsigned char Server_Rx_Buffer[Server_Rx_Buffer_Size];


	void* ThreadClient();
	int aunthenticated_client_socket_fd; 
	sockaddr_in aunthenticated_client_address;
	pthread_t client_thread = NULL; //thread to attend the authenticated client
	volatile bool client_running = false;
	static void* client_thread_helper(void* context)
	{
		return ((MasterApp*)context)->ThreadClient();
	}
	AES128 AES_Client;
	Frame framer;

	const static int Client_Rx_Buffer_Size = 511;
	unsigned char Client_Rx_Buffer[Client_Rx_Buffer_Size + 1];
	
	const static int Client_Tx_Buffer_Size = 512;
	unsigned char Client_Tx_Buffer[Client_Tx_Buffer_Size + 16];//Add 16 to let space for possible 15 padded bytes for encription
	

	enum ProcessorStatus { Starting, Authenticating, Working };
	AES128 AES_Server;
	void GenerateServerKey(unsigned char* server_key);
	bool AuthenticateClient(std::string version, std::string frota, std::string user, std::string password);
	unsigned char Hex2Byte(unsigned char* buff);

	int client_rx_frame_index = 0;
	void ParseRawRx(unsigned char* buffer, int length);
	std::string ExecuteCommand(std::string cmd, uint16_t FrameId);
	std::string ExecuteCommand_GOTO(vector<string> fields, uint16_t FrameId);
	std::string ExecuteCommand_RESET(vector<string> fields, uint16_t FrameId);
	std::string ExecuteCommand_SETMODE(vector<string> fields, uint16_t FrameId);
	std::string ExecuteCommand_POWER(vector<string> fields, uint16_t FrameId);

	void SendCommandACK(std::string msg, uint16_t FrameId);
	void Send(unsigned char* buffer, int length);
	std::string Execute(const char* cmd);
};

