#include <cstdio>
#include <iostream>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "LineNavigator.h"
#include "err.h"


LineNavigator::LineNavigator()
{


}

LineNavigator::~LineNavigator()
{
}

int LineNavigator::Init(std::string _port_name, int _baudrate, int _left_offset, int _right_offset, double _left_factor, double _right_factor, double _angle_factor)
{
	port_name = _port_name;
	baudrate = _baudrate;
	left_offset = _left_offset;
	right_offset = _right_offset;
	left_factor = _left_factor;
	right_factor = _right_factor;
	angle_factor = _angle_factor;

	return Open();
}

int LineNavigator::Open()
{

	//Open the serial port
	port_fd = open(port_name.c_str(), O_RDWR | O_NOCTTY);
	if (port_fd == -1)
		return err::ERR_OPENING_SERIAL_PORT;

	termios port_settings;
	bzero(&port_settings, sizeof(port_settings)); // clear struct for new port settings 

	//CS8     : 8n1 (8bit,no parity,1 stopbit)
	//CLOCAL  : local connection, no modem contol
	//CREAD   : enable receiving characters
	port_settings.c_cflag = CS8 | CLOCAL | CREAD;

	// Set the baud rate for both input and output.
	if (cfsetspeed(&port_settings, baudrate))
		return err::ERR_SETTING_BAUDRATE;

	//IGNPAR  : ignore bytes with parity errors
	port_settings.c_iflag = IGNPAR;

	//Raw output.
	port_settings.c_oflag = 0;

	//ICANON  : enable canonical input disable all echo functionality, and don't send signals to calling program
	port_settings.c_lflag = ICANON;

	//initialize all control characters 
	port_settings.c_cc[VINTR] = 0;		/* Ctrl-c */
	port_settings.c_cc[VQUIT] = 0;		/* Ctrl-\ */
	port_settings.c_cc[VERASE] = 0;		/* del */
	port_settings.c_cc[VKILL] = 0;		/* @ */
	port_settings.c_cc[VEOF] = 4;		/* Ctrl-d */
	port_settings.c_cc[VTIME] = 0;		/* inter-character timer unused */
	port_settings.c_cc[VMIN] = 1;		/* blocking read until 1 character arrives */
	port_settings.c_cc[VSWTC] = 0;		/* '\0' */
	port_settings.c_cc[VSTART] = 0;		/* Ctrl-q */
	port_settings.c_cc[VSTOP] = 0;		/* Ctrl-s */
	port_settings.c_cc[VSUSP] = 0;		/* Ctrl-z */
	port_settings.c_cc[VEOL] = 0;		/* '\0' */
	port_settings.c_cc[VREPRINT] = 0;	/* Ctrl-r */
	port_settings.c_cc[VDISCARD] = 0;	/* Ctrl-u */
	port_settings.c_cc[VWERASE] = 0;	/* Ctrl-w */
	port_settings.c_cc[VLNEXT] = 0;		/* Ctrl-v */
	port_settings.c_cc[VEOL2] = 0;		/* '\0' */

	//Clean the modem line
	if (tcflush(port_fd, TCIFLUSH) < 0)
		return err::ERR_SETTINGS_SERIAL_PORT_CONFIGURATION;

	// Apply the settings.
	if (tcsetattr(port_fd, TCSANOW, &port_settings) < 0)
		return err::ERR_SETTINGS_SERIAL_PORT_CONFIGURATION;

	port_timeout.tv_sec = 2;
	port_timeout.tv_usec = 0;

	FD_ZERO(&port_set); /* clear the set */
	FD_SET(port_fd, &port_set); /* add our file descriptor to the set */

	return 0;
}


void LineNavigator::Close()
{
	if (port_fd > -1)
		close(port_fd);
}


int LineNavigator::Read(LineNavigator::Data* data)
{
	int left_raw_low = 541;
	int left_raw_high = 766;
	double left_table[] = { 10.0, 9.5, 9.3, 9.0, 8.5, 8.3, 8.0, 7.8, 7.7, 7.5, 7.4, 7.3, 7.1, 7.0, 6.9, 6.8, 6.8, 6.7, 6.6, 6.5, 6.4, 6.4, 6.3, 6.3, 6.2, 6.1, 6.1, 6.0, 6.0, 5.9, 5.9, 5.8, 5.8, 5.8, 5.7, 5.7, 5.6, 5.6, 5.5, 5.5, 5.5, 5.5, 5.4, 5.4, 5.4, 5.4, 5.3, 5.3, 5.3, 5.3, 5.2, 5.2, 5.2, 5.2, 5.1, 5.1, 5.1, 5.1, 5.0, 5.0, 5.0, 4.9, 4.9, 4.9, 4.9, 4.8, 4.8, 4.8, 4.8, 4.7, 4.7, 4.7, 4.6, 4.6, 4.6, 4.6, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.4, 4.4, 4.4, 4.4, 4.4, 4.4, 4.4, 4.4, 4.4, 4.4, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.2, 4.2, 4.2, 4.2, 4.2, 4.2, 4.2, 4.2, 4.2, 4.2, 4.1, 4.1, 4.1, 4.1, 4.1, 4.1, 4.1, 4.1, 4.1, 4.1, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5, };
	//Items = 225

	int right_raw_low = 588;
	int right_raw_high = 763;
	double right_table[] = { 9.5, 9.0, 8.5, 8.0, 7.5, 7.0, 6.5, 6.0, 5.5, 5.5, 5.4, 5.4, 5.4, 5.4, 5.0, 5.0, 5.0, 5.0, 5.0, 4.9, 4.9, 4.9, 4.9, 4.9, 4.9, 4.9, 4.9, 4.8, 4.8, 4.8, 4.8, 4.8, 4.8, 4.8, 4.8, 4.8, 4.7, 4.7, 4.7, 4.7, 4.7, 4.7, 4.7, 4.7, 4.6, 4.6, 4.6, 4.6, 4.6, 4.6, 4.6, 4.6, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.4, 4.4, 4.4, 4.4, 4.4, 4.4, 4.4, 4.4, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.2, 4.2, 4.2, 4.2, 4.2, 4.2, 4.2, 4.1, 4.1, 4.1, 4.1, 4.1, 4.1, 4.1, 4.1, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.9, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.8, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5, };
	//Items = 175


	int left_raw, right_raw;



	int result = 0;
	int r = 0;

	char cmd = '$';
	write(port_fd, &cmd, 1);



	//int rv = select(port_fd + 1, &port_set, NULL, NULL, &port_timeout);
	int rv = select(FD_SETSIZE, &port_set, NULL, NULL, &port_timeout);

	if (rv == -1)
	{
		result = err::ERR_READ_ERROR;
	}
	else if (rv == 0)
	{
		result = err::ERR_READ_TIMEOUT;
	}
	else
	{
		r = read(port_fd, port_rx_buffer, port_rx_buffer_size);
		//port_rx_buffer[r] = 0;
		//printf("%s\n", port_rx_buffer);
		if (r > 4)
		{
			result = VerifyChecksum(port_rx_buffer);
			if (result == 0)
			{
				char* p = port_rx_buffer;
				char* p_start;

				p = strchr(p, ',') + 1;
				data->IdFrame = atoi(p);

				p = strchr(p, ',') + 1;
				left_raw = atoi(p);
				data->raw_left = left_raw;

				//Limit the interval
				if (left_raw < left_raw_low)
					left_raw = left_raw_low;
				else if (left_raw >= left_raw_high)
					left_raw = left_raw_high - 1;

				data->Left = left_factor * (left_table[left_raw - left_raw_low] + left_offset);

				p = strchr(p, ',') + 1;
				right_raw = atoi(p);
				data->raw_right = right_raw;

				//Limit the interval
				if (right_raw < right_raw_low)
					right_raw = right_raw_low;
				else if (right_raw >= right_raw_high)
					right_raw = right_raw_high - 1;

				data->Right = right_factor * (right_table[right_raw - right_raw_low] + right_offset);

				data->angle = angle_factor * (data->Left - data->Right);

				//limit angle 
				if (data->angle > 80)
					data->angle = 80;
				if (data->angle < -80)
					data->angle = -80;
			}
			else
			{
				result = err::ERR_CHECKSUM_ERROR;
			}
		}
		else
		{
			result = err::ERR_INVALID_FRAME;
		}
	}

	data->Status = result;

	return result;
}



int LineNavigator::VerifyChecksum(const char* message)
{
	int i = 0;
	int c = port_rx_buffer_size - 8;
	char p;
	unsigned char sum = 0;
	++message;
	while (((p = *message++) != '*') && i < c)
	{
		sum ^= p;
		i++;
	}

	unsigned char checksum = (uint8_t)strtol(message, NULL, 16);


	if (sum != checksum)
	{
		return err::ERR_INVALID_CHECKSUM;
	}
	else
	{
		return 0;
	}
}