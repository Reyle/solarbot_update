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

#include "GPS.h"
#include "err.h"
#include <math.h>

using namespace std;


GPS::GPS()
{
}


GPS::~GPS()
{
}

int GPS::Open(const char * port_name, int baudrate)
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
	if(tcflush(port_fd, TCIFLUSH) < 0)
		return err::ERR_SETTINGS_SERIAL_PORT_CONFIGURATION;

	// Apply the settings.
	if (tcsetattr(port_fd, TCSANOW, &port_settings) < 0)
		return err::ERR_SETTINGS_SERIAL_PORT_CONFIGURATION;
	
	

	return 0;
}

void GPS::Close()
{
	if (port_fd > -1)
		close(port_fd);
}

void GPS::Reset()
{
	//Reset de GPS
	int DTR_flag = TIOCM_DTR;
	ioctl(port_fd, TIOCMBIS, &DTR_flag);//Set DTR pin
	usleep(20000);
	ioctl(port_fd, TIOCMBIC, &DTR_flag);//Clear DTR pin
	usleep(20000);
	ioctl(port_fd, TIOCMBIS, &DTR_flag);//Set DTR pin
}

int GPS::getLocation(GPS::Location* location)
{
	
	location->status = 'I';

	unsigned char status = NMEA_EMPTY;
	while (status != NMEA_COMPLETED) {
		int r = read(port_fd, port_rx_buffer, SERIAL_PORT_RX_BUFFER_SIZE);

		//if (r >= SERIAL_PORT_RX_BUFFER_SIZE)
		//{
		//	printf("ERROR GPS SENTENCE TOO LONG!!!\r");
		//}
		//else
		//{
		//	port_rx_buffer[r] = 0x00;
		//	printf(port_rx_buffer, "%s");
		//}

		if (r > 0)
		{
			switch (nmea_get_message_type(port_rx_buffer))
			{
			case NMEA_GPGGA:
				nmea_parse_gpgga(port_rx_buffer, location);
				status |= NMEA_GPGGA;
				break;
			case NMEA_GPRMC:
				nmea_parse_gprmc(port_rx_buffer, location);
				status |= NMEA_GPRMC;
				break;
			}
		}
	}

	return 0;
}

void GPS::Write(char* buffer, int buffer_length)
{
	write(port_fd, buffer, buffer_length);
}

uint16_t GPS::year(uint32_t date)
{
	return 2000 + (date % 100);
}

uint8_t GPS::month(uint32_t date)
{
	return (date / 100) % 100;
}

uint8_t GPS::day(uint32_t date)
{
	return  date / 10000;
}

uint8_t GPS::hour(uint32_t time)
{
	return time / 1000000;
}

uint8_t GPS::minute(uint32_t time)
{
	return (time / 10000) % 100;
}

uint8_t GPS::second(uint32_t time)
{
	return (time / 100) % 100;
}

uint8_t GPS::centisecond(uint32_t time)
{
	return time % 100;
}


void GPS::nmea_parse_gpgga(char *nmea, Location *loc)
{
	char *p = nmea;

	p = strchr(p, ',') + 1; //skip time (take from GPRMC)
	p = strchr(p, ',') + 1; //skip latitude (take from GPRMC)
	p = strchr(p, ',') + 1; //skip latitude dir (take from GPRMC)
	p = strchr(p, ',') + 1; //skip longitude (take from GPRMC)
	p = strchr(p, ',') + 1; //skip longitude dir (take from GPRMC)

	//Quality
	p = strchr(p, ',') + 1;
	loc->quality = (uint8_t)atoi(p);

	//Number of satellites in use
	p = strchr(p, ',') + 1;
	loc->satellites = (uint8_t)atoi(p);

	//Horizontal dilution of precision
	p = strchr(p, ',') + 1;
	loc->hdop = atof(p);

	//Altitude
	p = strchr(p, ',') + 1;
	loc->altitude = atof(p);

	p = strchr(p, ',') + 1; //skip altitude units (always is Meters?)

	//Ondulation
	p = strchr(p, ',') + 1;
	loc->ondulation = atof(p);

	p = strchr(p, ',') + 1; //skip undulation units (always is Meters?)

	//age
 	p = strchr(p, ',') + 1;
	loc->age = atoi(p);

	//Station ID
	p = strchr(p, ',') + 1;
	loc->StationID = atoi(p);
}

void GPS::nmea_parse_gprmc(char *nmea, Location *loc)
{
	char *p = nmea;
	char* lat_start;
	char* lon_start;

	p = strchr(p, ',') + 1; //time
	uint32_t time = 100 * (uint32_t)atol(p);
	while (isdigit(*p)) 
		++p;
	if (*p == '.' && isdigit(p[1]))
	{
		time += 10 * (p[1] - '0');
		if (isdigit(p[2]))
			time += p[2] - '0';
	}
	loc->time = time;
	//loc->hour = time / 1000000;
	//loc->minute = (time / 10000) % 100;
	//loc->second = (time / 100) % 100;
	//loc->centisecond = time % 100;


	p = strchr(p, ',') + 1; //Status
	loc->status = p[0];

	p = strchr(p, ',') + 1; //Latitude
	lat_start = p;


	p = strchr(p, ',') + 1;
	if (loc->status == 'A' )
		loc->latitude = gps_to_degree(lat_start, p[0]);
	else
		loc->latitude = 0.0L;

	p = strchr(p, ',') + 1; //Longitude
	lon_start = p;

	p = strchr(p, ',') + 1;
	if (loc->status == 'A')
		loc->longitude = gps_to_degree(lon_start, p[0]);
	else
		loc->longitude = 0.0L;

	p = strchr(p, ',') + 1; //Speed
	loc->speed = atof(p);

	p = strchr(p, ',') + 1; //Course
	loc->course = atof(p);

	p = strchr(p, ',') + 1; //Date
	loc->date = atol(p);
	//uint32_t date = atol(p);
	//loc->year = 2000 + (date % 100);
	//loc->month = (date / 100) % 100;
	//loc->day = date / 10000;
}

double GPS::gps_to_degree(char * buffer, char sing)
{
	double result = 0;
	if (sing == 'N' || sing == 'S')
	{
		result = 10 * (buffer[0] - 48) + (buffer[1] - 48);
		result+= (strtold(buffer + 2, NULL)/60.0L);

		if (sing == 'S')
			result = -result;
	}
	else if (sing == 'E' || sing == 'W')
	{
		result = (100 * (buffer[0] - 48)) + (10*((buffer[1] - 48))) + (buffer[2] - 48);
		result += (strtold(buffer + 3, NULL) / 60.0L);

		if (sing == 'W')
			result = -result;
	}
	else
	{
		result = 0.0L;
	}

	return result;
}

/**
* Get the message type (GPGGA, GPRMC, etc..)
*
* This function filters out also wrong packages (invalid checksum)
*
* @param message The NMEA message
* @return The type of message if it is valid
*/
uint8_t GPS::nmea_get_message_type(const char *message)
{
	if (nmea_valid_checksum(message) != NMEA_EMPTY) {
		return NMEA_CHECKSUM_ERR;
	}

	if (strstr(message, NMEA_GPGGA_STR) != NULL) {
		return NMEA_GPGGA;
	}

	if (strstr(message, NMEA_GPRMC_STR) != NULL) {
		return NMEA_GPRMC;
	}

	return NMEA_UNKNOWN;
}

unsigned char GPS::nmea_valid_checksum(const char *message) {
	//unsigned char checksum = (uint8_t)strtol(strchr(message, '*') + 1, NULL, 16);

	int i = 0;
	int c = SERIAL_PORT_RX_BUFFER_SIZE - 8;
	char p;
	unsigned char sum = 0;
	++message;
	while (((p = *message++) != '*') && i<c) 
	{
		sum ^= p;
		i++;
	}

	unsigned char checksum = (uint8_t)strtol(message, NULL, 16);


	if (sum != checksum) {
		return NMEA_CHECKSUM_ERR;
	}

	return NMEA_EMPTY;
}



