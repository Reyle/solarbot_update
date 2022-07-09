#include "GPSConsumer.h"

GPSConsumer::GPSConsumer(std::string host, std::string port)
{
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET; // IPv4 
	inet_pton(AF_INET, host.c_str(), &(address.sin_addr));//Convert string to IP
	address.sin_port = htons((uint16_t)atoi(port.c_str()));
}
