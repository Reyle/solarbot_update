#include <iostream>
#include "Telemetry.h"
#include "SocketException.h"
#include <string.h>
#include <vector>
#include <string>
#include <sstream>

Telemetry::Telemetry()
{
}

Telemetry::~Telemetry()
{
}

int Telemetry::Start()
{
	return Start((char*)this->ID.c_str(), this->local_port, (char*)this->server_host.c_str(), this->server_port, (char*)this->server_user.c_str(), (char*)this->server_password.c_str());
}

int Telemetry::Start(char* telemetry_ID, uint16_t telemetry_local_port, char* telemetry_server_host, uint16_t telemetry_server_port, char* telemetry_server_user, char* telemetry_server_password)
{
	//unsigned char key[] = { 0xB0, 0x5D, 0x51, 0x1F, 0x23, 0xE7, 0x0B, 0xC7, 0xEC, 0xFD, 0x08, 0x3B, 0x81, 0x48, 0x69, 0x80 };
	//AES_Telemetry_Server.SetKey(key);

	FrameID = 0;
	m_port = telemetry_local_port;
	this->telemetry_ID = telemetry_ID;
	this->telemetry_server_host = telemetry_server_host;
	this->telemetry_server_port = telemetry_server_port;
	this->telemetry_server_user = telemetry_server_user;
	this->telemetry_server_password = telemetry_server_password;

	Telemetry_Server_Tx_Buffer_Length = sizeof(Frame) + 6;


	server_working = true;
	int r = pthread_create(&server_thread, NULL, &Telemetry::server_thread_helper, this);

	if (r != 0)
		return r;

	if (telemetry_server_host != NULL && strlen(telemetry_server_host) > 0 && telemetry_server_port > 0)
	{
		r = pthread_create(&telemetric_server_thread, NULL, &Telemetry::telemetric_server_thread_helper, this);
	}

	return r;

}

void Telemetry::Stop()
{
	server_working = false;
	socket_telemetry_server.close();
	client_socket.Close();
	sleep(1);
}

void Telemetry::SendFrame(unsigned char FrameType, unsigned char* bytes, uint16_t length)
{
	int blocks = 0;
	if (FrameType == 0x01)
		blocks = 5;
	else
		return;

	uint16_t crc = 0x0000;
	unsigned char* p_crc = (unsigned char*)&crc;
	unsigned char* p_length = (unsigned char*)&length;

	try
	{
		//Transmit to Telemetry Server
		if (!Telemetry_Server_Tx_Buffer_Flag)
		{
			//Make the Header		
			Telemetry_Tx_Raw_Buffer[0] = 0x7E;
			Telemetry_Tx_Raw_Buffer[1] = *p_length;
			Telemetry_Tx_Raw_Buffer[2] = *(p_length + 1);
			Telemetry_Tx_Raw_Buffer[3] = FrameType;

			//Encrypt the frame payload with the Telemetry Server AES
			AES_Telemetry_Server.Encrypt(bytes, Telemetry_Tx_Raw_Buffer + 4, blocks);

			//Calculate and add the CRC16
			crc = 0x0273; //TODO CALCULAR EL CRC
			Telemetry_Tx_Raw_Buffer[4 + length] = *p_crc;
			Telemetry_Tx_Raw_Buffer[5 + length] = *(p_crc + 1);

			//Copy to the Telemetric Tx Server Buffer
			memcpy(Telemetry_Server_Tx_Buffer, Telemetry_Tx_Raw_Buffer, length + 6);

			//Mark to the transmit thread sends it
			Telemetry_Server_Tx_Buffer_Flag = true;
		}
	}
	catch (SocketException & er)
	{
		std::cout << "ERROR: Telemetry send to server. " << er.description() << ".\n";
	}

	try
	{
		//Transmit to Client
		if (client_socket.Valid() && client_is_authenticated)
		{
			//Make the header
			Server_Tx_Buffer[0] = 0x7E;
			Server_Tx_Buffer[1] = *p_length;
			Server_Tx_Buffer[2] = *(p_length + 1);
			Server_Tx_Buffer[3] = FrameType;

			//Encrypt the payload with the client AES
			AES_Server.Encrypt(bytes, Server_Tx_Buffer + 4, blocks);

			//Calculate and add the CRC16
			crc = 0x0273; //TODO CALCULAR EL CRC
			Server_Tx_Buffer[4 + length] = *p_crc;
			Server_Tx_Buffer[5 + length] = *(p_crc + 1);

			//Transmit to Client
			client_socket.Write(Server_Tx_Buffer, length + 6);
		}
	}
	catch (SocketException & er)
	{
		std::cout << "ERROR: Telemetry send to client. " << er.description() << ".\n";
		client_socket.close();
	}
}

void Telemetry::GenerateServerKey(unsigned char* server_key)
{
	int sv = 11;
	for (int i = 0; i < 64; i++)
	{
		server_key[i] = (unsigned char)(rand() % (256));
	}

	unsigned char key[16];
	for (int i = 0; i < 16; i++)
	{
		key[i] = server_key[sv];
		sv = server_key[sv] / 4;
	}

	AES_Server.SetKey(key);

	//printf("Autehntication key = ");
	//for (int i = 0; i < 16; i++)
	//	printf("%02X", key[i]);
	//printf("\n");
}

void* Telemetry::ThreadServer()
{
	bool IsConnected = false;

	while (server_working)
	{
		try
		{
			ServerSocket server_socket(m_port);

			try
			{
				while (server_working)
				{					
					server_socket.accept(client_socket);
					IsConnected = true;
					client_is_authenticated = false;
					ClientStatus = ProcessorStatus::Starting;
					client_socket.set_read_timeout(10);

					try
					{
						while (IsConnected && client_socket.is_valid())
						{
							if (ClientStatus == ProcessorStatus::Working)
							{
								usleep(10000);
							}
							else if (ClientStatus == ProcessorStatus::Authenticating)
							{
								int c = client_socket.Read(Server_Rx_Buffer, Server_Rx_Buffer_Size);

								AES_Server.Decrypt(Server_Rx_Buffer, Server_Rx_Buffer, c / 16);

								std::string str(reinterpret_cast<char const*>(Server_Rx_Buffer), c);
								std::stringstream ss(str);
								std::vector<std::string> fields;
								while (ss.good())
								{
									std::string substr;
									getline(ss, substr, ',');
									fields.push_back(substr);
								}

								if (fields.size() > 5 && fields[0] == "SolarBot" && fields[4].size() == 32)
								{
									if (AuthenticateClient(fields[0], fields[1], fields[2], fields[3]))
									{
										unsigned char key[16];
										for (int i = 0; i < 16; i++)
										{
											key[i] = Hex2Byte((unsigned char*)(fields[4].c_str() + (i * 2)));
										}

										AES_Server.SetKey(key);
										ClientStatus = ProcessorStatus::Working;
										client_is_authenticated = true;
									}
									else
									{
										IsConnected = false;
									}
								}
								else
								{
									IsConnected = false;
								}
							}
							else if (ClientStatus == ProcessorStatus::Starting)
							{
								client_is_authenticated = false;
								std::string data;
								client_socket >> data;
								//std::cout << data;

								if (data == "SOLARBOT")
								{
									//Generate ServerKey and transmit
									unsigned char server_key[64];
									GenerateServerKey(server_key);
									client_socket.Write(server_key, 64);
									ClientStatus = ProcessorStatus::Authenticating;
								}
								else
								{
									IsConnected = false;
								}

							}
							
						}
					}
					catch (SocketException& eee)
					{
						std::cout << "Merda: " << eee.description() << ".\n";
						IsConnected = false;
					}				
					
					client_socket.Close();
				}
			}
			catch (SocketException& ee)
			{
				std::cout << "Exception on Thread was caught:" << ee.description() << "\n";
			}
		}
		catch (SocketException& e)
		{
			std::cout << "Exception on Server was caught:" << e.description() << "\n";
		}

		usleep(500000);
	}

	return nullptr;
}

bool Telemetry::AuthenticateClient(std::string version, std::string frota, std::string user, std::string password)
{
	bool result = version == "SolarBot";


	//if (version == "100")
	//    result = db.AuthenticateClient(frota, user, password) == 1;

	////if(!result)
	////{
	////    AddLogError("Authentication failed. version=" + version + "  frota=" + frota + "  user=" + user);
	////}

	return result;
}

unsigned char Telemetry::Hex2Byte(unsigned char* buff)
{
	unsigned char b = 0x00;

	int i = buff[0] - 0x30;
	if (i >= 0 && i < 0x0A)
		b = i << 4;
	else
	{
		if (i > 0x10 && i < 0x17)
		{
			i -= 0x07;
			b = i << 4;
		}
	}

	i = buff[1] - 0x30;
	if (i >= 0 && i < 0x0A)
		b += i;
	else
	{
		if (i > 0x10 && i < 0x17)
		{
			i -= 0x07;
			b +=i;
		}
	}

	return b;
}

//void* Telemetry::ThreadClient(int socket_id)
//{
//
//	//ServerSocket s = *socket;
//	//while (server_working)
//	//{
//	//	s << "Hola Thread!\n";
//	//	usleep(500000);
//	//}
//	char* buffer = "Merda OK?\n";
//
//	while (server_working)
//	{
//		send(socket_id, buffer, 10, 0);
//		sleep(1);
//	}
//
//	return nullptr;
//}

void* Telemetry::ThreadTelemetricServer()
{
	bool IsConnected = false;
	bool IsAutenticated = true;
	while (server_working)
	{
		try
		{
			if (!socket_telemetry_server.is_valid())
			{
				if (socket_telemetry_server.create())
				{
					socket_telemetry_server.set_read_timeout(10);
				}
				else
				{
					sleep(1);
				}
			}
			else
			{
				if (!IsConnected)
				{
					IsConnected = socket_telemetry_server.connect(telemetry_server_host, telemetry_server_port);
					if (!IsConnected)
						sleep(1);

					IsAutenticated = false;
					Telemetry_Server_Tx_Buffer_Flag = true; //to avoid the SendFrame overwrite the tx buffer
				}
				else if (!IsAutenticated)
				{
					Telemetry_Server_Tx_Buffer_Flag = true; //to avoid the SendFrame overwrite the tx buffer
					try
					{
						int c = sprintf((char*)Telemetry_Server_Tx_Buffer, "SOLARBOT");
						socket_telemetry_server.write(Telemetry_Server_Tx_Buffer, c);
						Telemetry_Server_Rx_Buffer_Length = socket_telemetry_server.read_with_status(Telemetry_Server_Rx_Buffer, Telemetry_Server_Rx_Buffer_Size);
						if (Telemetry_Server_Rx_Buffer_Length == 64)
						{
							//Extract the AES key
							int sv = 11;
							for (int i = 0; i < 16; i++)
							{
								Telemetry_Server_AES_Key[i] = Telemetry_Server_Rx_Buffer[sv];
								sv = Telemetry_Server_Rx_Buffer[sv] / 4;
							}
							AES_Telemetry_Server.SetKey(Telemetry_Server_AES_Key);

							int c = sprintf((char*)Telemetry_Server_Tx_Buffer, "SolarBot,%s,%s,%s,", telemetry_ID, telemetry_server_user, telemetry_server_password);

							//Generate a new AES random key
							srand(time(NULL));
							unsigned char b;
							for (int i = 0; i < 16; i++)
							{
								b = (unsigned char)(rand() % (256));
								c += sprintf((char*)Telemetry_Server_Tx_Buffer + c, "%02X", b);
								Telemetry_Server_AES_Key[i] = b;
							}
							c += sprintf((char*)Telemetry_Server_Tx_Buffer + c, ",END");

							//printf("%s\n", Telemetry_Server_Tx_Buffer);

							while (c % 16 != 0)
								Telemetry_Server_Tx_Buffer[c++] = 0x00;

							//Encryptic
							AES_Telemetry_Server.Encrypt(Telemetry_Server_Tx_Buffer, Telemetry_Server_Tx_Buffer, c / 16);

							//Send
							socket_telemetry_server.write(Telemetry_Server_Tx_Buffer, c);

							//Apply the new AES key
							AES_Telemetry_Server.SetKey(Telemetry_Server_AES_Key);

							IsAutenticated = true;
							Telemetry_Server_Tx_Buffer_Flag = false; //to allow the SendFrame write to the tx buffer
						}
						else
						{
							socket_telemetry_server.close();
							IsConnected = false;
						}
					}
					catch (SocketException er)
					{
						Telemetry_Server_Rx_Buffer_Length = -1;
					}
					//}
					//else if (Status == ProcessorStatus::Starting)
					//{

					//}
					//else if (Status == ProcessorStatus::Starting)
					//{

					//}
				}
				else
				{
					if (Telemetry_Server_Tx_Buffer_Flag)
					{
						try
						{
							socket_telemetry_server.write(Telemetry_Server_Tx_Buffer, Telemetry_Server_Tx_Buffer_Length);
							Telemetry_Server_Rx_Buffer_Length = socket_telemetry_server.read_with_status(Telemetry_Server_Rx_Buffer, Telemetry_Server_Rx_Buffer_Size);
						}
						catch (SocketException er)
						{
							Telemetry_Server_Rx_Buffer_Length = -1;
						}

						if (Telemetry_Server_Rx_Buffer_Length <= 0)
						{
							socket_telemetry_server.close();
							IsConnected = false;
						}

						Telemetry_Server_Tx_Buffer_Flag = false;
						
					}
				}
			}
		}
		catch (SocketException err)
		{
			sleep(1);
		}

		usleep(100000);
	}
	
	return nullptr;
}
