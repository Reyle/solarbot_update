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

#include "ControlUnit.h"
#include "err.h"




ControlUnit::ControlUnit()
{
}


ControlUnit::~ControlUnit()
{
}


int ControlUnit::Open(const char* port_name, int baudrate)
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

	port_timeout.tv_sec = 1;
	port_timeout.tv_usec = 1000;

	FD_ZERO(&port_set); /* clear the set */
	FD_SET(port_fd, &port_set); /* add our file descriptor to the set */

	return 0;
}

int ControlUnit::Open()
{
	return Open("/dev/ttyDriver", 57600);
}

void ControlUnit::Close()
{
	if (port_fd > -1)
		close(port_fd);
}

int ControlUnit::Write(int left, int right)
{
	int result = 0;
	if (!RemoteControllerActivate)
		result = DirectWrite(left, right);

	return result;
}

int ControlUnit::RealRead()
{
	mtx_read.lock();
	int result = 0;

	try
	{		
		int r;
		port_timeout.tv_sec = 1;
		port_timeout.tv_usec = 1000;
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
			port_rx_buffer[r] = 0x00;
			//std::cout << "RXS: " << std::string(port_rx_buffer) << std::endl;

			if (r > 4)
			{
				if (port_rx_buffer[0] == '$' && port_rx_buffer[1] == 'E' && port_rx_buffer[2] == 'R')
				{
					std::cout << "Driver Notified Error: " << std::string(port_rx_buffer) << std::endl;
				}
				else
				{
					result = VerifyChecksum(port_rx_buffer);
					if (result == 0)
					{
						//std::cout << "OK rx driver" << std::endl;
						char* p = port_rx_buffer;
						char* p_start;

						if (strncmp(port_rx_buffer, "$C01", 4) == 0)
						{

							//$C01, IdFrame, LeftSpeed, RightSpeed, LeftAlarm, RightAlarm, RemoteControlled*Checksum
							p = strchr(p, ',') + 1;
							data.IdFrame = atoi(p);

							p = strchr(p, ',') + 1;
							data.LeftSpeedPulses = atoi(p);

							p = strchr(p, ',') + 1;
							data.RightSpeedPulses = atoi(p);

							p = strchr(p, ',') + 1;
							data.LeftAlarm = atoi(p) == 1;

							p = strchr(p, ',') + 1;
							data.RightAlarm = atoi(p) == 1;

							p = strchr(p, ',') + 1;
							data.RemoteControlled = atoi(p) == 1;

							data.LeftSpeed = data.LeftSpeedPulses * SpeedFactor;
							data.RightSpeed = data.RightSpeedPulses * SpeedFactor;

							data.last_update = std::chrono::steady_clock::now();
						}
						if (strncmp(port_rx_buffer, "$A01", 4) == 0)
						{
							p = strchr(p, ',') + 1;
							uint8_t IdFrame = atoi(p);

							p = strchr(p, ',') + 1;
							int id = atoi(p);

							p = strchr(p, ',') + 1;
							int16_t vertical_angle = atoi(p);

							p = strchr(p, ',') + 1;
							int16_t horizontal_angle = atoi(p);

							printf("Rx Arm %u,%d,%d,%d\n", IdFrame, id, vertical_angle, horizontal_angle);
						}
					}
					else
					{
						std::cerr << "ERROR rx driver checksum" << std::endl;
					}
				}
			}
			else
			{
				std::cerr << "ERROR rx driver invalid frame" << std::endl;
				result = err::ERR_INVALID_FRAME;
			}
		}
		data.Status = result;
	}
	catch (...) {}

	mtx_read.unlock();

	return result;
}

int ControlUnit::Read()
{
	int result = 0;

	//Do not send Read command if the last read from the CMD command is younger than 800ms
	if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - data.last_update).count() > 800)
	{

		if (mtx_write.try_lock())
		{
			try
			{
				int c = sprintf(port_tx_buffer, "$GET*56\n");
				//unsigned char chk = 0;
				//int i = 0;
				int r = 0;
				//for (i = 1; i < c; i++)
				//	chk ^= port_tx_buffer[i];
				//c += sprintf(port_tx_buffer + c, "*%02X\n", chk);
				//tcdrain(port_fd); // Wait until a previus transmission ends
				//tcflush(port_fd, TCOFLUSH); // Clear write buffer
				write(port_fd, port_tx_buffer, c);
				//printf("TX2: %s", port_tx_buffer);
			}
			catch (...)
			{
				result = -1;
			}
			mtx_write.unlock();
		}

		if (result == 0)
			result = RealRead();
	}

	return result;
}

void ControlUnit::RemoteControllerWrite(int left, int right)
{
	RemoteControllerActivate = true;
	DirectWrite(left, right);
}

int ControlUnit::WriteArm(int16_t vertical_speed, int16_t horizontal_speed)
{
	int result = 0;

	mtx_write.lock();
	try
	{
		int c = sprintf(port_tx_buffer, "$CAM,0,%d,%d", vertical_speed, horizontal_speed);
		unsigned char chk = 0;
		int i = 0;
		int r = 0;
		for (i = 1; i < c; i++)
			chk ^= port_tx_buffer[i];
		c += sprintf(port_tx_buffer + c, "*%02X\n", chk);
		//tcdrain(port_fd); // Wait until a previus transmission ends
		//tcflush(port_fd, TCOFLUSH); // Clear write buffer
		write(port_fd, port_tx_buffer, c);
		//printf("TX3: %s", port_tx_buffer);		
	}
	catch (...)
	{
		result = -1;
	}
	mtx_write.unlock();

	if (result == 0)
	{
		//int rv = select(FD_SETSIZE, &port_set, NULL, NULL, &port_timeout);
		//
		//if (rv == -1)
		//{
		//	result = err::ERR_READ_ERROR;
		//}
		//else if (rv == 0)
		//{
		//	result = err::ERR_READ_TIMEOUT;
		//}
		//else
		//{
		//	r = read(port_fd, port_rx_buffer, port_rx_buffer_size);
		//
		//	if (r > 4)
		//	{
		//		result = VerifyChecksum(port_rx_buffer);
		//		if (result == 0)
		//		{
		//			//std::cout << "OK rx driver" << std::endl;
		//			char* p = port_rx_buffer;
		//			char* p_start;
		//
		//			//$C01, IdFrame, LeftSpeed, RightSpeed, LeftAlarm, RightAlarm, RemoteControlled*Checksum
		//			p = strchr(p, ',') + 1;
		//			data.IdFrame = atoi(p);
		//
		//			p = strchr(p, ',') + 1;
		//			data.LeftSpeedPulses = atoi(p);
		//
		//			p = strchr(p, ',') + 1;
		//			data.RightSpeedPulses = atoi(p);
		//
		//			p = strchr(p, ',') + 1;
		//			data.LeftAlarm = atoi(p) == 1;
		//
		//			p = strchr(p, ',') + 1;
		//			data.RightAlarm = atoi(p) == 1;
		//
		//			p = strchr(p, ',') + 1;
		//			data.RemoteControlled = atoi(p) == 1;
		//		}
		//		else
		//		{
		//			std::cout << "ERROR rx driver" << std::endl;
		//			result = err::ERR_CHECKSUM_ERROR;
		//		}
		//	}
		//	else
		//	{
		//		result = err::ERR_INVALID_FRAME;
		//	}
		//}
		//
		//data.Status = result;
	}


	return result;
}

int ControlUnit::DirectWrite(int left, int right)
{
	int result = 0;

	mtx_write.lock();
	try
	{
		int c = sprintf(port_tx_buffer, "$CMD,%d,%d", left, right);
		unsigned char chk = 0;
		int i = 0;
		int r = 0;
		for (i = 1; i < c; i++)
			chk ^= port_tx_buffer[i];
		c += sprintf(port_tx_buffer + c, "*%02X\n", chk);
		//tcdrain(port_fd); // Wait until a previus transmission ends
		//tcflush(port_fd, TCOFLUSH); // Clear write buffer
		write(port_fd, port_tx_buffer, c);
		//printf("TX1: %s", port_tx_buffer);		
	}
	catch (...)
	{
		result = -1;
	}
	mtx_write.unlock();

	if(result == 0)
		result = RealRead();

	data.Status = result;

	return result;
}

int ControlUnit::VerifyChecksum(const char* message)
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

