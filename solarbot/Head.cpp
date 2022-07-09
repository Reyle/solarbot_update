#include "Head.h"
#include "err.h"
#include "Solarbot.h"

Head::Head()
{
	RxRawFrameIndex = 0;
	RxRawFrameLength = 0;
	escaping = false;
	rxflag = false;
}



int Head::Open(char* device, int baudrate)
{
	port_fd = open(device, O_RDWR | O_NOCTTY);
	if (port_fd == -1)
		return err::ERR_OPENING_SERIAL_PORT;
	
	termios port_settings;
	memset(&port_settings, 0, sizeof(port_settings));

	if (tcgetattr(port_fd, &port_settings) < 0)
	{
		return err::ERR_GETTING_PORT_SETTINGS;
	}

	// @NOTE - termios.c_line is not a standard element of the termios
	// structure, (as per the Single Unix Specification 3).
	port_settings.c_line = '\0';

	// Ignore Break conditions on input.
	port_settings.c_iflag = IGNBRK;

	port_settings.c_oflag = 0;

	// Enable the receiver (CREAD) and ignore modem control lines (CLOCAL).
	port_settings.c_cflag |= CREAD | CLOCAL;

	port_settings.c_lflag = 0;

	// Set the baud rate for both input and output.
	if (cfsetspeed(&port_settings, baudrate))
	{
		return err::ERR_SETTING_BAUDRATE;
	}

	port_settings.c_iflag &= ~ISTRIP; // Clear the ISTRIP flag.

	port_settings.c_cflag &= ~CSIZE;                               // Clear all CSIZE bits.
	port_settings.c_cflag |= static_cast<tcflag_t>(CS8); // Set the character size.


	// Flush the input and output buffers associated with the port.
	if (tcflush(port_fd, TCIOFLUSH) < 0)
	{
		return err::ERR_FLUSHING;
	}

	////FLOW_CONTROL_HARDWARE:
	//port_settings.c_iflag &= ~(IXON | IXOFF);
	//port_settings.c_cflag |= CRTSCTS;
	//port_settings.c_cc[VSTART] = _POSIX_VDISABLE;
	//port_settings.c_cc[VSTOP] = _POSIX_VDISABLE;

	// NO FLOW CONTROL
	port_settings.c_cflag &= ~CRTSCTS;

	//PARITY_NONE:
	port_settings.c_cflag &= ~PARENB;
	port_settings.c_iflag |= IGNPAR;

	//STOP_BITS_1:
	port_settings.c_cflag &= ~CSTOPB;

	port_settings.c_cc[VMIN] = 1;
	port_settings.c_cc[VTIME] = 0;


	// Apply the modified settings.
	if (tcsetattr(port_fd, TCSANOW, &port_settings) < 0)
	{
		return err::ERR_SETTINGS_SERIAL_PORT_CONFIGURATION;
	}

	////Set exclusive access
	//if (ioctl(port_fd, TIOCEXCL) == -1)
	//{
	//	return err::ERR_SETTING_EXCLUSIVE_ACCESS;
	//}

	//Active and deactivate the RST line to archive a hardware reset
	int DTR_flag = TIOCM_DTR;
	ioctl(port_fd, TIOCMBIC, &DTR_flag);//Clear RTS pin
	usleep(100000);
	ioctl(port_fd, TIOCMBIS, &DTR_flag);//Set RTS pin
	usleep(100000);
	ioctl(port_fd, TIOCMBIC, &DTR_flag);//Clear RTS pin


	int r = 1;
	working = true;
	rxflag = false;
	r = pthread_create(&pthreadport, NULL, &Head::port_read_helper, this);
	if (r != 0)
	{
		return err::ERR_CREATING_SERIAL_PORT_READ_THREAD;
	}

	return 0;
}

int Head::Open()
{
	return Open("/dev/ttyHead", 57600);
}

void Head::Close()
{
	if (port_fd > -1)
		close(port_fd);
}

int Head::ReadSensors()
{
	rxflag = false;
	write(port_fd, "$", 1);
	int timeout_count = 0;
	while (true)
	{
		usleep(1000);
		//std::this_thread::yield();

		timeout_count++;
		if (rxflag || timeout_count > 100)
			break;
	}

	if (timeout_count < 100)
	{
		data = *(struct Data*)RxRawFrame;

		data.Heading += declination;

		if (data.Heading >= 360)
			data.Heading -= 360;

		if (data.Heading < 0)
			data.Heading += 360;

		return 0;
	}
	else
	{
		return err::ERR_INVALID_FRAME;
	}	
}

int Head::CalibrateMagneticHeading(int samples)
{
	//TODO: Implement!!!
	return 0;

}

void Head::SetMagneticDeclination(float declination)
{
	this->declination = declination; 
}

float Head::getTemperatureC()
{
	//TODO: Implement !!!!!
	return 0.0;
}

void Head::SetEyes(Emotions e, bool auto_reverse_animation, bool continue_animation, bool return_to_neutral)
{
	int result = 0;
	try
	{
		int c = sprintf(port_tx_buffer, "%d,%d%d%d\r\n", e, auto_reverse_animation, continue_animation, return_to_neutral);
		write(port_fd, port_tx_buffer, c);
	}
	catch (...)
	{
		result = -1;
	}
}

void* Head::port_read()
{
	while (working)
	{
		int r, i;
		uint8_t a;

		if (!rxflag)
		{
			r = read(port_fd, port_rx_buffer, port_rx_buffer_size);

			if (r > 0)
			{
				for (i = 0; i < r; i++)
				{
					a = port_rx_buffer[i];

					if (a == 0x7E)
					{
						RxRawFrameIndex = 0;
						escaping = false;
						RxRawFrameLength = 0;
						rx_crc8.CRC_Init();
					}
					else
					{
						if (a == 0x7D)
						{
							escaping = 1;
						}
						else
						{
							if (escaping)
							{
								escaping = 0;
								a ^= 0x20;
							}

							if (RxRawFrameIndex == 0 && RxRawFrameLength == 0)
							{
								RxRawFrameLength = a;
								rx_crc8.CRC_Update(a);
							}
							else
							{
								if (RxRawFrameIndex == RxRawFrameLength)
								{
									RxRawFrameIndex = 0;

									if (rx_crc8.crc == a)
									{
										rxflag = true;
									}
								}
								else
								{
									RxRawFrame[RxRawFrameIndex] = a;
									rx_crc8.CRC_Update(a);

									RxRawFrameIndex++;
									if (RxRawFrameIndex >= port_rx_buffer_size)
										RxRawFrameIndex = 0;
								}
							}
						}
					}
				}

			}
		}

		//std::this_thread::yield();
		usleep(5000);
	}
	return nullptr;
}
