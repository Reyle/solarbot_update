#ifndef CRC16_H
#define CRC16_H

#include <cstdint>

//Source: https://os.mbed.com/users/EmLa/code/CRC16//file/585ead300cab/CRC16.h/

/*
This class implements a basic CRC16 (17Bits) using polynomial 0x8005 (X^16 + X^15 + X^2 +1).
CRC-16-IBM, using in Bisync, Modbus, USB, ANSI X3.28, SIA DC-07, many others; also known as CRC-16-ANSI
*/

/*
	//Example of use:
	char testdata[]= "123456789";
	CRC16 *myCRC = new CRC16();
	unsigned short resultCRC = myCRC->calculateCRC16(testdata,9); //9 is the length of the character array //
	pc.printf("%x",resultCRC);
*/

class CRC16
{
private:
	static const unsigned int SHIFTER;
	static const unsigned short TABLE[];

public:
	uint16_t calculate(unsigned char* input, int lenght);
};
#endif