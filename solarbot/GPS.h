#pragma once
#include <ctime>
#include <inttypes.h>


#define SERIAL_PORT_RX_BUFFER_SIZE 255

#define NMEA_EMPTY 0x00
#define NMEA_GPRMC 0x01
#define NMEA_GPRMC_STR "$GNRMC"
#define NMEA_GPGGA 0x02
#define NMEA_GPGGA_STR "$GNGGA"
#define NMEA_UNKNOWN 0x00
#define NMEA_COMPLETED 0x03

#define NMEA_CHECKSUM_ERR 0x80
#define NMEA_MESSAGE_ERR 0xC0

/*
Para configurar el GPS:
$PUBX,40,GLL,0,0,0,0,0,0*5C<CR><LF>  
$PUBX,40,GSV,0,0,0,0,0,0*59<CR><LF>
$PUBX,40,GSA,0,0,0,0,0,0*4E<CR><LF>
$PUBX,40,VTG,0,0,0,0,0,0*5E<CR><LF>

//Poner intervalo de 250ms
B5 62 06 08 06 00 FA 00 01 00 01 00 10 96

//Guardar la configuración
B5 62 06 09 0D 00 00 00 00 00 FF FF 00 00 00 00 00 00 17 31 BF

*/

//struct gpgga {
//	uint8_t quality; //Quality 	
//	uint8_t satellites;// Number of satellites	
//	float hdop; //Horizontal dilution of precision in meters
//	double altitude; // Altitude above mean sea level in meters
//	int age;//Age of correction data in seconds
//	int StationID; //Id of the differencial base station
//};
//typedef struct gpgga gpgga_t;
//
//struct gprmc {
//	time_t time;
//	char status;
//	double latitude;
//	char latitude_direction;
//	double longitude;
//	char longitude_direction;
//	double speed; 
//	double course;	
//};
//typedef struct gprmc gprmc_t;


class GPS
{
public:

#pragma pack(push)
#pragma pack(1)
	struct Location {
		uint32_t date;
		uint32_t time;
		double latitude;
		double longitude;
		double speed;
		double course;
		char status; //I= Invalid, A=autonomus or only one antenna, R= The course was calculated using two antennas
		unsigned char quality;//0=invalid, 1=GPS fix(SPS), 2=DGPS fix, 3=PPS fix, 4=Real Time Kinematic, 5=Float RTK, 6=estimated(dead reckoning), 7=Manual input mode, 8=Simulation mode
		unsigned char satellites;// Number of satellites	
		float hdop; //Horizontal dilution of precision in meters
		double altitude; // Altitude above mean sea level
		float ondulation;
		int age;//Age of correction data in seconds
		int StationID; //Id of the differencial base station
	};
#pragma pack(pop)

	GPS();
	~GPS();

	int Open(const char* port_name, int baudrate);
	void Close();
	void Reset();

	int getLocation(Location* location);

	void Write(char* buffer, int buffer_length);

	//Parse funtions
	uint16_t year(uint32_t date);
	uint8_t month(uint32_t date);
	uint8_t day(uint32_t date);
	uint8_t hour(uint32_t time);
	uint8_t minute(uint32_t time);
	uint8_t second(uint32_t time);
	uint8_t centisecond(uint32_t time);



private:

	int port_fd;
	char port_rx_buffer[SERIAL_PORT_RX_BUFFER_SIZE + 1];

	unsigned char nmea_get_message_type(const char *);
	unsigned char nmea_valid_checksum(const char *);
	void nmea_parse_gpgga(char *, Location *);
	void nmea_parse_gprmc(char *, Location *);
	double gps_to_degree(char* buffer, char sing);
	

};

