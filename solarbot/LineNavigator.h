#pragma once
#include <sys/types.h>
#include <sys/select.h>
#include <string>

class LineNavigator
{
public:

	//Start raw value = 534
//End raw value = 770


	struct Data {
		int Status;
		unsigned char IdFrame;
		int raw_left;
		int raw_right;
		double Left;
		double Right;
		double angle;
	};

	LineNavigator();
	~LineNavigator();
	int Init(std::string _port_name, int _baudrate, int _left_offset = 0, int _right_offset = 0, double _left_factor = 1.0, double _right_factor = 1.0, double _angle_factor = 25.0);
	int Open();
	void Close();


	int Read(Data* data);



	std::string port_name;
	int baudrate = 9600;

	int left_offset = 0;
	int right_offset = 0;

	double left_factor = 1.0;
	double right_factor = 1.0;

	double angle_factor = 25.0;


private:
	static const int port_rx_buffer_size = 100;
	int port_fd;
	char port_rx_buffer[port_rx_buffer_size + 1];
	int VerifyChecksum(const char* message);
	fd_set port_set;
	struct timeval port_timeout;
};

