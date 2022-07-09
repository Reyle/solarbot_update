#include "MasterApp.h"


MasterApp::MasterApp(uint16_t port, Solarbot* solarbot, volatile int* ctrl)
{
	this->port = port;
	this->solarbot = solarbot;
	this->ctrl = ctrl;

	//Start Server Thread
	pthread_create(&server_thread, NULL, &MasterApp::server_thread_helper, this);

	
}

std::string MasterApp::SetVisualMode()
{
	return std::string();
}

std::string MasterApp::SetWaypointsMode(std::vector<Navigator::Waypoint>)
{
	return std::string();
}

std::string MasterApp::SetLineMode()
{
	return std::string();
}

void MasterApp::Close()
{
	try
	{
		close(server_socket_fd);
		close(aunthenticated_client_socket_fd);
	}
	catch (...) {}
}


void* MasterApp::ThreadServer()
{
	bool IsConnected = false;
	bool client_is_authenticated = false;
	ProcessorStatus ClientStatus = ProcessorStatus::Starting;
	
	
	sockaddr_in server_addr;
	sockaddr_in new_client_addr;

	while (ctrl)
	{
		try
		{
			server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
			if (server_socket_fd < 0)
			{
				std::cerr << "ERROR: Cannot open socket for Command Server" << std::endl;
				exit(EXIT_FAILURE);
				return nullptr;
			}

			//Create Listened Address
			bzero((char*)&server_addr, sizeof(server_addr));
			server_addr.sin_family = AF_INET;
			server_addr.sin_addr.s_addr = INADDR_ANY;
			server_addr.sin_port = htons(port);

			//Bind socket
			if (bind(server_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
			{
				std::cerr << "ERROR: Cannot bind the Command Socket" << std::endl;
				exit(EXIT_FAILURE);
				return nullptr;
			}

			if (listen(server_socket_fd, MAXCONNECTIONS) < 0)
			{
				std::cerr << "ERROR: Cannot listen on Command Socket" << std::endl;
				exit(EXIT_FAILURE);
				return nullptr;
			}


			try
			{
				while (ctrl)
				{
					int new_client_addr_length = sizeof(new_client_addr);
					int new_client_socket_fd = accept(server_socket_fd, (sockaddr*)&new_client_addr, (socklen_t*)&new_client_addr_length);

					if (new_client_socket_fd > -1)
					{
						//Set Rx timeout
						struct timeval tv;
						tv.tv_sec = 10; //10 seconds
						tv.tv_usec = 0;
						setsockopt(new_client_socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

						IsConnected = true;
						client_is_authenticated = false;
						ClientStatus = ProcessorStatus::Starting;
						try
						{
							while (IsConnected && new_client_socket_fd > -1)
							{
								if (ClientStatus == ProcessorStatus::Working)
								{
									usleep(10000); //The program cannot reach here!!!
								}
								else if (ClientStatus == ProcessorStatus::Authenticating)
								{
									int c = read(new_client_socket_fd, Server_Rx_Buffer, Server_Rx_Buffer_Size);

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

									if (fields.size() > 5 && fields[4].size() == 32)
									{
										if (AuthenticateClient(fields[0], fields[1], fields[2], fields[3]))
										{
											unsigned char key[16];
											for (int i = 0; i < 16; i++)
											{
												key[i] = Hex2Byte((unsigned char*)(fields[4].c_str() + (i * 2)));
											}

											ClientStatus = ProcessorStatus::Working;
											client_is_authenticated = true;

											int rr = 0;
											try {
												client_running = false; //to try to quit the thread (in case its running)
												close(aunthenticated_client_socket_fd); //Close the socket (It is create an abort in the thread, I hope)

												////Just in case lets wait at least 1 second (I am so generous!!!)
												//if (client_thread != NULL)
												//{
												//	struct timespec ts;
												//	ts.tv_sec += 1;
												//	rr = pthread_timedjoin_np(client_thread, NULL, &ts);

												//	//Is still alive?!... lets kill it!
												//	if (rr != 0)
												//		pthread_cancel(client_thread); //Que se joda
												//}
											}
											catch (...) {}



											AES_Client.SetKey(key); //Set the new key

											//Save the socket
											aunthenticated_client_socket_fd = new_client_socket_fd;
											aunthenticated_client_address = new_client_addr;

											pthread_create(&client_thread, NULL, &MasterApp::client_thread_helper, this); //Create a client thread

											break; //To exit and be ready for a possible new client

										}
										else
										{
											close(new_client_socket_fd);
											IsConnected = false;
										}
									}
									else
									{
										close(new_client_socket_fd);
										IsConnected = false;
									}
								}
								else if (ClientStatus == ProcessorStatus::Starting)
								{
									client_is_authenticated = false;
									int c = read(new_client_socket_fd, Server_Rx_Buffer, Server_Rx_Buffer_Size - 1);
									Server_Rx_Buffer[c] = 0x00;
									std::string data(reinterpret_cast<char*>(Server_Rx_Buffer), c);
									
									if (data == "SOLARBOT")
									{
										//Generate ServerKey and transmit
										unsigned char server_key[64];
										GenerateServerKey(server_key);
										write(new_client_socket_fd, server_key, 64);
										ClientStatus = ProcessorStatus::Authenticating;
									}
									else
									{
										close(new_client_socket_fd);
										IsConnected = false;
									}

								}

							}
						}
						catch (...)
						{
							std::cout << "EROR: Merda\n";
							close(new_client_socket_fd);
							IsConnected = false;
						}
					}

				}
			}
			catch (...)
			{
				std::cout << "ERROR: Exception on Thread was caught" << std::endl;
			}
		}
		catch (...)
		{
			std::cout << "ERROR: Exception on Server was caught" << std::endl;
		}

		usleep(500000);
	}

	return nullptr;
}

void* MasterApp::ThreadClient()
{
	while (ctrl)
	{
		try 
		{
			int c = read(aunthenticated_client_socket_fd, Client_Rx_Buffer, Client_Rx_Buffer_Size);
			if (c > 0)
			{
				//printf("RX Length=%d\n",c);
				AES_Client.Decrypt(Client_Rx_Buffer, Client_Rx_Buffer, c / 16);
				ParseRawRx(Client_Rx_Buffer, c);
			}
		}
		catch (...) {}

		usleep(100000);
	}
	return nullptr;
}


void MasterApp::GenerateServerKey(unsigned char* server_key)
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

bool MasterApp::AuthenticateClient(std::string version, std::string frota, std::string user, std::string password)
{
	bool result = (version == solarbot->CMD_version) && (frota == solarbot->MyID) && (user == solarbot->CMD_user) && (password == solarbot->CMD_password);

	if (result)
	{
		std::cout << "REMOTE CONTROL CONNECTED!" << std::endl;
	}
	else
	{
		std::cout << "ERROR1: REMOTE CONTROL version=" << solarbot->CMD_version << " ID=" << solarbot->MyID << " user=" << solarbot->CMD_user << " password=" << solarbot->CMD_password << std::endl;
		std::cout << "ERROR2: REMOTE CONTROL version=" << version << " ID=" << frota << " user=" << user << " password=" << password << std::endl;
	}
	return result;
}

unsigned char MasterApp::Hex2Byte(unsigned char* buff)
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
			b += i;
		}
	}

	return b;
}

void MasterApp::ParseRawRx(unsigned char* buffer, int length)
{
	//for (int i = 0; i < length; i++)
	//	printf("%02X ", buffer[i]);
	//printf("\n");

	unsigned char a;
	bool escaping = false;
	for (int i = 0; i < length; i++)
	{
		if (framer.DecodeFrame(buffer[i]))
		{
			if (framer.rx_FrameType == FramesTypes::Ping)
			{
				printf("RX Ping %u\n", framer.rx_FrameId);
				int c = framer.GeneratePingACKFrame(Client_Tx_Buffer, Client_Tx_Buffer_Size, framer.rx_FrameId);
				Send(Client_Tx_Buffer, c);
			}
			else if (framer.rx_FrameType == FramesTypes::Command)
			{
				framer.frame_buffer[framer.rx_PayloadLength + 8] = 0x00;

				ExecuteCommand(std::string((char*)(framer.frame_buffer + 8)), framer.rx_FrameId);
			}
		}		
	}
}

std::string MasterApp::ExecuteCommand(std::string cmd, uint16_t FrameId)
{
	std::cout << cmd << std::endl;

	istringstream iss(cmd);
	vector<string> tokens{ istream_iterator<string>{iss}, istream_iterator<string>{} };

	for (int i = 0; i < tokens.size(); i++)
		std::cout << i << "=" << tokens[i] << std::endl;

	if (tokens.size() > 0)
	{
		if (tokens[0] == "GOTO")
		{
			return ExecuteCommand_GOTO(tokens, FrameId);
		}
		else if (tokens[0] == "RESET")
		{
			return ExecuteCommand_RESET(tokens, FrameId);
		}
		else if (tokens[0] == "SETMODE")
		{
			return ExecuteCommand_SETMODE(tokens, FrameId);
		}
		else if (tokens[0] == "POWER")
		{
			return ExecuteCommand_POWER(tokens, FrameId);
		}
		else
		{
			SendCommandACK("ERROR: No Command", FrameId);
		}
	}
	else
	{
		SendCommandACK("ERROR: Parsing Command", FrameId);
	}

	

	return std::string();
}

void MasterApp::SendCommandACK(std::string msg, uint16_t FrameId)
{
	int c = framer.GenerateCommandACKFrame(Client_Tx_Buffer, Client_Tx_Buffer_Size, FrameId, msg);
	Send(Client_Tx_Buffer, c);
}

std::string MasterApp::ExecuteCommand_GOTO(vector<string> fields, uint16_t FrameId)
{
	//	0 = GOTO
	//	1 = -87.138391923
	//	2 = 40.5562028947
	//	3 = 90
	//	4 = 0
	//	5 = 1000
	//	6 = 0

	if (solarbot->IsLocationGood())
	{
		double my_latitude = solarbot->location.latitude;
		double my_longitude = solarbot->location.longitude;
		double dest_longitude = atof(fields[1].c_str());
		double dest_latitude = atof(fields[2].c_str());
		double heading = atof(fields[3].c_str());
		int maneuver = atoi(fields[4].c_str());
		int speed = atoi(fields[5].c_str());
		double offset = atof(fields[6].c_str());

		//python /home/solarbot/route_optimization/main.py --current_point=-87.1382050819 40.5557702121 --desired_point=-87.138391923 40.5562028947 --direction=90 --maneuver=0 --speed=1000 --offset=0
		char ask[255];
		int c = sprintf(ask, "python3 /home/solarbot/route_optimization/main.py --current_point=%0.10f %0.10f --desired_point=%0.10f %0.10f --direction=%0.1f --maneuver=%d --speed=%d --offset=%0.2f > /home/solarbot/waypoints/temp.txt", my_longitude, my_latitude, dest_longitude, dest_latitude, heading, maneuver, speed, offset);
		printf("%s\n", ask);

		std::string strWaypoints = Execute(ask);
		if (solarbot->navigator->LoadWaypoints("/home/solarbot/waypoints/temp.txt") == err::err_codes::OK)
		{
			solarbot->navigator->start_waypoint_index = -1;
			SendCommandACK("OK", FrameId);
			solarbot->setNavigationMode("waypoints");
			solarbot->navigating = true;
		}
		else
		{
			SendCommandACK("ERROR: Loading generated waypoints file", FrameId);
		}
	}
	else
	{
		SendCommandACK("ERROR: Current Location is not good", FrameId);
	}
	return std::string();
}

std::string MasterApp::ExecuteCommand_RESET(vector<string> fields, uint16_t FrameId)
{
	if (fields[1] == "GPS")
	{
		solarbot->gps->Reset();
		solarbot->gps2->Reset();
		SendCommandACK("OK", FrameId);
	}
	else
	{
		SendCommandACK("ERROR: Invalid RESET command", FrameId);
	}
	return std::string();
}

std::string MasterApp::ExecuteCommand_SETMODE(vector<string> fields, uint16_t FrameId)
{
	Navigator::Modes new_mode = solarbot->navigator->String2Mode(fields[1]);

	if (new_mode == Navigator::Modes::ModeWaypoints)
	{
		if (solarbot->navigator->waypoints.size() > 0)
		{
			solarbot->navigator->start_waypoint_index = -1; //To find the closets point again
			solarbot->navigator->Mode = new_mode;
			solarbot->navigating = true;
			SendCommandACK("OK", FrameId);
		}
		else
		{
			//Error there is not any waypoints, so go to standby and stop navigation
			solarbot->navigator->Mode = Navigator::Modes::Standby;
			solarbot->navigating = false;
			SendCommandACK("ERROR: There is not any waypoints", FrameId);
		}
	}
	else if (new_mode == Navigator::Modes::ModeVisual)
	{
		int r = solarbot->InitModeVisual();
		if (r == 0)
		{
			solarbot->navigator->Mode = new_mode;
			solarbot->navigating = true;
			SendCommandACK("OK", FrameId);
		}
		else
		{
			//Error setting mode, so go to standby and stop navigation
			solarbot->navigator->Mode = Navigator::Modes::Standby;
			solarbot->navigating = false;
			SendCommandACK("ERROR: InitModeVisual", FrameId);
		}
	}
	else if (new_mode == Navigator::Modes::ModeLine)
	{
		int r = solarbot->InitModeLine();
		if (r == 0)
		{
			solarbot->navigator->Mode = new_mode;
			solarbot->navigating = true;
			SendCommandACK("OK", FrameId);
		}
		else
		{
			//Error setting mode, so go to standby and stop navigation
			solarbot->navigator->Mode = Navigator::Modes::Standby;
			solarbot->navigating = false;
			SendCommandACK("ERROR: InitModeLine", FrameId);
		}
	}
	else
	{
		solarbot->navigator->Mode = Navigator::Modes::Standby;
		solarbot->navigating = false;
		SendCommandACK("OK", FrameId);
	}

	return std::string();
}

std::string MasterApp::ExecuteCommand_POWER(vector<string> fields, uint16_t FrameId)
{
	solarbot->power_unit->Write(fields[1].c_str(), strlen(fields[1].c_str()));
	//write(solarbot->power_unit->port_fd, fields[1].c_str(), strlen(fields[1].c_str()));
	SendCommandACK("OK", FrameId);
}

void MasterApp::Send(unsigned char* buffer, int length)
{
	int blocks = 1 + (length / 16); //Always at least one block

	//Pad at the enad with zeros
	int pad_index_start = 16 * (blocks - 1);
	while (length < pad_index_start)
	{
		buffer[length] = 0x00;
		length++;
	}

	AES_Client.Encrypt(Client_Tx_Buffer, Client_Tx_Buffer, blocks);
	send(aunthenticated_client_socket_fd, Client_Tx_Buffer, 16*blocks, 0);
}

std::string MasterApp::Execute(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;
}


