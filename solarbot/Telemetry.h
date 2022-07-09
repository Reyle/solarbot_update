#pragma once

// Listen and accept only one connection of local telemetry, this allow to direct connect to the robot to get the telemetry.
// Also transmit the telemetry to the Telemetry Server, to allow anyone access the telemetry remotely

#include <pthread.h>
#include "GPS.h"
#include "PowerUnit.h"
#include "ServerSocket.h"
#include "AES128.h"



class Telemetry
{
public:
	//typedef void* (*THREADFUNCPTR)(void*);

	struct Frame {
		
		unsigned char FrameID;
		unsigned char GPSStatus;
		unsigned char FrameValid1;
		unsigned char FrameValid2;

		
		int GPSLatitude;

		int GPSLongitude;

		short GPSAltitude;
		short GPSSpeed;

		short GPSBearing;
		unsigned short GPSBaseID;

		short MagneticBearing;
		short PitchAngle;

		short RollAngle;
		short Reserved2;

		unsigned char PowerSensorsStatus;
		unsigned char PowerSensorsIdFrame;
		short PanelVoltage;

		short BatteryVoltage;
		short PanelPower;

		unsigned char StopPressed;
		unsigned char RemoteControlled;
		short BatteryPower;

		short ElectronicPower;
		short LeftPower;

		short RightPower;
		short LeftMotorSpeed;

		short RightMotorSpeed;
		unsigned char LeftAlarm;
		unsigned char RightAlarm;

		short LeftMotorCommand;
		short RightMotorCommand;

		int CurrentDestinationPointIndex;

		int CurrentDestinationPointLatitude;

		int CurrentDestinationPointLongitude;

		short CurrentDestinationPointSpeed;
		uint16_t STATUS;

		uint32_t RESERVED1;
		uint32_t RESERVED2;
	};

	struct FrameACK
	{
		unsigned char FrameType;
		unsigned char FrameID;
		unsigned char Reserved1;
		unsigned char Reserved2;
	};

	Telemetry();
	//Telemetry(int port);
	~Telemetry();

	volatile bool server_working;

	const static int Telemetry_Tx_Buffer_Size = 512;
	unsigned char Telemetry_Tx_Raw_Buffer[Telemetry_Tx_Buffer_Size];
	volatile bool Telemetry_Server_Tx_Buffer_Flag = false;
	unsigned char Telemetry_Server_Tx_Buffer[Telemetry_Tx_Buffer_Size];
	int Telemetry_Server_Tx_Buffer_Length = 0;

	const static int Telemetry_Server_Rx_Buffer_Size = 512;
	unsigned char Telemetry_Server_Rx_Buffer[Telemetry_Server_Rx_Buffer_Size];
	int Telemetry_Server_Rx_Buffer_Length = 0;

	const static int Server_Rx_Buffer_Size = 512;
	unsigned char Server_Rx_Buffer[Server_Rx_Buffer_Size];

	const static int Server_Tx_Buffer_Size = 512;
	unsigned char Server_Tx_Buffer[Server_Tx_Buffer_Size];

	
	int local_port = 2010;
	std::string ID = "0000";
	std::string server_host = "";
	uint16_t server_port = 0;
	std::string server_user = "";
	std::string server_password = "";

	int Start();
	int Start(char* ID, uint16_t telemetry_local_port, char* telemetry_server_host = NULL, uint16_t telemetry_server_port = 0, char* telemetry_server_user = NULL, char* telemetry_server_password = NULL);
	void Stop();
	void SendFrame(unsigned char FrameType, unsigned char* bytes, uint16_t length);

	unsigned char FrameID;
	

private:

	AES128 AES_Telemetry_Server;
	AES128 AES_Server;

	void GenerateServerKey(unsigned char* server_key);

	enum ProcessorStatus { Starting, Authenticating, Working };

	Socket socket_telemetry_server;

	// using to pass parameter to the thread that attent the local client
	struct Client_Thread_Parameter
	{
		Telemetry* telemetry;
		int socket_id;
	};


	int m_port;
	char* telemetry_ID = NULL;
	char* telemetry_server_host = NULL;
	int16_t telemetry_server_port;
	char* telemetry_server_user = NULL;
	char* telemetry_server_password = NULL;

	pthread_t server_thread; //thread to listen local connection
	pthread_t client_thread; //thread to attent the local client
	pthread_t telemetric_server_thread; //thread 
	bool client_is_authenticated = false;
	ProcessorStatus ClientStatus = ProcessorStatus::Starting;



	void* ThreadServer();
	//void* ThreadClient(int socket_int);
	void* ThreadTelemetricServer();
	bool AuthenticateClient(std::string version, std::string frota, std::string user, std::string password);
	unsigned char Hex2Byte(unsigned char* buff);

	ServerSocket client_socket;

	static void* server_thread_helper(void* context)
	{
		return ((Telemetry*)context)->ThreadServer();
	}

	//static void* client_thread_helper(void* context)
	//{
	//	Client_Thread_Parameter* parameter = (Client_Thread_Parameter*)context;
	//	int socket_id = parameter->socket_id;
	//	return ((Telemetry*)parameter->telemetry)->ThreadClient(socket_id);
	//}

	static void* telemetric_server_thread_helper(void* context)
	{
		return ((Telemetry*)context)->ThreadTelemetricServer();
		//Telemetry* p = (Telemetry*)context;
		//return p->ThreadTelemetricServer(p->host, p->port, p->user, p->password);
	}

	
	ProcessorStatus Status = ProcessorStatus::Starting;
	unsigned char Telemetry_Server_AES_Key[16];
};

