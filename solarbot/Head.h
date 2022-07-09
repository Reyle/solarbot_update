#pragma once

#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <pthread.h>
#include <thread> //yield()
#include <cstdint>
#include <unistd.h>
#include <math.h>

#include "CRC8.h"

class Head
{
public:
	//struct Data
	//{
	//	int16_t AcX;
	//	int16_t AcY;
	//	int16_t AcZ;
	//	int16_t GyX;
	//	int16_t GyY;
	//	int16_t GyZ;
	//	int16_t MgX;
	//	int16_t MgY;
	//	int16_t MgZ;
	//};
	//struct Data data;

	struct Data
	{
		float Heading;
		float Pitch;
		float Roll;
		uint16_t ErrorCount;
	};
	struct Data data;

	enum Emotions
	{
		E_NEUTRAL,		//0
		E_BLINK,		//1
		E_WINK,			//2
		E_LOOK_L,		//3
		E_LOOK_R,		//4
		E_LOOK_U,		//5
		E_LOOK_D,		//6
		E_ANGRY,		//7
		E_SAD,			//8
		E_VIL,			//9
		E_EVIL2,		//10
		E_SQUINT,		//11
		E_E_DEAD,		//12
		E_E_SCAN_UD,	//13
		E_SCAN_LR,		//14
		E_CRAZY,		//15
		E_SLEEP,		//16
		E_X,			//17
		E_PROHIBIT,		//18
		E_ARROW_LEFT,	//19
		E_ARROW_RIGHT,	//20
		E_LOADING,		//21
	};

	Head();
	int Open(char* device, int baudrate);	
	int Open();
	void Close();
	int ReadSensors();
	int CalibrateMagneticHeading(int samples);
	void SetMagneticDeclination(float declination);
	float getTemperatureC();
	void SetEyes(Emotions e, bool auto_reverse_animation = false, bool continue_animation = false, bool return_to_neutral = false);

	double declination = 0.0;

private:
	CRC8 rx_crc8;

	static const int rx_raw_frame_size = 100;
	uint8_t RxRawFrame[rx_raw_frame_size];
	volatile int RxRawFrameIndex;
	volatile int RxRawFrameLength;
	volatile bool rxflag;
	volatile bool escaping;

	volatile bool working = false;
	int port_fd;
	static const int port_rx_buffer_size = 100;
	char port_rx_buffer[port_rx_buffer_size];
	void* port_read();
	pthread_t pthreadport;
	static void* port_read_helper(void* context)
	{
		return ((Head*)context)->port_read();
	}

	const static int port_tx_buffer_size = 32;
	char port_tx_buffer[port_tx_buffer_size];
};

