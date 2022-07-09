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
#include <chrono>  // for high_resolution_clock
#include <math.h>

#include "PowerUnit.h"
#include "err.h"


PowerUnit::PowerUnit()
{
}

PowerUnit::~PowerUnit()
{
}

int PowerUnit::Open(const char* port_name, int baudrate)
{
	//Open the serial port
	port_fd = open(port_name, O_RDWR | O_NOCTTY);
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

int PowerUnit::Open()
{
	return Open("/dev/ttyPowerUnit", 115200);
}

void PowerUnit::Close()
{
	if (port_fd > -1)
		close(port_fd);
}

void PowerUnit::Write(const char* buffer, int size)
{
	printf("POWER UNIT, Write:%s\n", buffer);
	write(port_fd, buffer, size);
}

int PowerUnit::Read(bool reset_motors)
{
	int result = 0;
	int r = 0;

	char* cmd;
	char* cmd_ok = "$CMD,0,0*4A\n";
	char* cmd_ok_AccessoriesON = "$CMD,1,0*4B\n";
	char* cmd_reset = "$CMD,0,1*4B\n"; //para resetear los motores

	int c = 12;
	if (reset_motors)
	{
		cmd = cmd_reset;
	}
	else
	{
		cmd = cmd_ok_AccessoriesON;
	}
	
	write(port_fd, cmd, c);

	
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
		while(r > 1 && port_rx_buffer[0] == '#')//to ignore any comment line
			r = read(port_fd, port_rx_buffer, port_rx_buffer_size);

		if (r > 4)
		{
			result = VerifyChecksum(port_rx_buffer);
			if (result == 0)
			{
				char* p = port_rx_buffer;
				char* p_start;

				//IdFrame, iVPanel, iVBattery, iPanel, iBattery, iElectronic, iLeft, iRight, iAccessories, iStop
				p = strchr(p, ',') + 1;
				data.IdFrame = atoi(p);

				p = strchr(p, ',') + 1;
				data.PanelVoltage = atoi(p) / 10.0f;

				p = strchr(p, ',') + 1;
				data.BatteryVoltage = atoi(p) / 10.0f;

				p = strchr(p, ',') + 1;
				data.PanelPower = atoi(p);

				p = strchr(p, ',') + 1;
				data.BatteryPower = atoi(p);

				p = strchr(p, ',') + 1;
				data.ElectronicPower = atoi(p);

				p = strchr(p, ',') + 1;
				data.LeftPower = atoi(p);

				p = strchr(p, ',') + 1;
				data.RightPower = atoi(p);

				p = strchr(p, ',') + 1;
				data.AccessoriesPower = atoi(p);

				p = strchr(p, ',') + 1;
				data.StopPressed = atoi(p) == 1;
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

	data.Status = result;
	
	return result;
}

int PowerUnit::VerifyChecksum(const char* message)
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