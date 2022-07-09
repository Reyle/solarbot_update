#pragma once

#include <iostream>
#include <string>

#include "opencv2/opencv.hpp"

#include "PracticalSocket.h"
#include "CRC16.h"

class UdpVideoSender
{
public:

	struct Header
	{
		int32_t PayloadLength;
		uint8_t FrameType;
		uint8_t Reserved;
		uint16_t FrameId;
	};

	std::string UdpServerHost = "";
	uint16_t UdpServerPort;
	int UdpServerPacketSize;
	PracticalSocket::UDPSocket sock;
	struct Header header;
	
	unsigned char* tx_buffer;
	int tx_buffer_index = 0;
	CRC16 crc16;

	UdpVideoSender(std::string UdpServerHost, uint16_t UdpServerPort, int UdpServerPacketSize = 512, int Quality = 80);
	void Send(Header header, unsigned char *payload);
	void SendImage(cv::Mat img);

private:
	std::vector<int> compression_params;
};

