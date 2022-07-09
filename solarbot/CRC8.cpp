#include "CRC8.h"

const unsigned char CRC8_Lookup[16] = {
	0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
	0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D
};


CRC8::CRC8()
{
}


CRC8::~CRC8()
{
}

//Inicializa el crc con 0xFF
void CRC8::CRC_Init()
{
	crc = 0xFF;
}

void CRC8::CRC_Update(unsigned char val)
{
	crc = (crc << 4) ^ CRC8_Lookup[((crc) ^ val) >> 4];
	crc = (crc << 4) ^ CRC8_Lookup[((crc >> 4) ^ val) & 0x0F];
}

unsigned char CRC8::GetCRC(char* buff, int length)
{
	CRC_Init();
	return GetCRC_Update(buff, length);
}

unsigned char CRC8::GetCRC_Update(char* buff, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{
		CRC_Update(buff[i]);
	}

	return crc;
}