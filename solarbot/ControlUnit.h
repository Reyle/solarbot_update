#pragma once
#include <stdint.h>
#include <mutex>          // std::mutex
#include <chrono>

class ControlUnit
{
public:
	ControlUnit();
	~ControlUnit();
	const float SpeedFactor = 0.0067272727272727F;
	struct DriverData {
		int Status;
		unsigned char IdFrame;
		int LeftSpeedPulses;
		int RightSpeedPulses;
		float LeftSpeed; // m/s
		float RightSpeed; //  m/s
		bool LeftAlarm;
		bool RightAlarm;
		bool RemoteControlled;
		std::chrono::steady_clock::time_point last_update;
	};

	struct DriverData data;

	volatile bool RemoteControllerActivate = false;

	int Open(const char* port_name, int baudrate);
	int Open();
	void Close();
	int Write(int left, int right);	
	int Read();
	void RemoteControllerWrite(int left, int right);
	int WriteArm(int16_t vertical_speed, int16_t horizontal_speed);
	


private:
	int port_fd;
	fd_set port_set;
	struct timeval port_timeout;
	const static int port_rx_buffer_size = 32;
	char port_rx_buffer[port_rx_buffer_size];

	const static int port_tx_buffer_size = 32;
	char port_tx_buffer[port_tx_buffer_size];
	std::mutex mtx_read;           // mutex for read
	std::mutex mtx_write;           // mutex for write
	int RealRead();
	int DirectWrite(int left, int right);
	int VerifyChecksum(const char* message);

};

