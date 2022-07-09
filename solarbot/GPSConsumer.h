#pragma once
#include <string>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>





class GPSConsumer
{
public:
	struct sockaddr_in address;

	GPSConsumer(std::string host, std::string port);
	
};

