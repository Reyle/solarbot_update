#pragma once
class CRC8
{
public:
	CRC8();
	~CRC8();

	unsigned char crc;

	//Inicializa el crc con 0xFF
	void CRC_Init(void);

	//Procesa el valor "val" para actualizar el crc
	void CRC_Update(unsigned char val);

	//Inicializa el crc (con 0xFF) y calcula el crc de "buff" por la longitud "length"
	unsigned char GetCRC(char* buff, int length);

	//Calcula el crc de "buff" por la longitud "length"
	unsigned char GetCRC_Update(char* buff, int length);
};

