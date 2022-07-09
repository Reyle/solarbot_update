#pragma once
#include <sys/types.h>
#include <sys/select.h>

class PowerUnit
{
public:

	struct PowerUnitData {
		int Status;
		unsigned char IdFrame;
		float PanelVoltage;
		float BatteryVoltage;
		short PanelPower;
		short BatteryPower;
		short ElectronicPower;
		short LeftPower;
		short RightPower;
		short AccessoriesPower;
		bool StopPressed;
	};

	struct PowerUnitData data;

	PowerUnit();
	~PowerUnit();
	int Open(const char* port_name, int baudrate);
	int Open();
	void Close();

	void Write(const char* buffer, int size);
	int Read(bool reset_motors = false);

	volatile int port_fd;

private:
	static const int port_rx_buffer_size = 100;	
	char port_rx_buffer[port_rx_buffer_size + 1];
	int VerifyChecksum(const char* message);
	fd_set port_set;
	struct timeval port_timeout;
};

