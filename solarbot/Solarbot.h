#pragma once
#include <iostream>
#include <string>
#include <vector>

#include "Head.h"
#include "Navigator.h"
#include "CamNavigator.h"
#include "PowerUnit.h"
#include "ControlUnit.h"
#include "Telemetry.h"
#include "Cam_Realsense.h"
#include "Detect_object.h"
#include "PracticalSocket.h"
#include "Cropmapping_Process.h"
#include "GPSConsumer.h"





class Solarbot
{
public:
	std::string MyID = "0000";
	std::string RTKServer_host = "192.168.0.0";
	std::string RTKServer_password = "";
	std::string RTKServer_asking_base = "0000";
	uint16_t RTKServer_port = 4002;

	std::string CMD_version = "SolarBot";
	std::string CMD_user = "user";
	std::string CMD_password = "password";



	std::string WaypointsFile = "";
	std::string NavigationCamera = "Center";
	std::string LineSensorPath = "/dev/ttyLine";

	bool reopen_head_on_error = false;

	Navigator* navigator;
	bool IsBrasil = false;
	//bool IsGPS2 = false;
	bool TransmitNavigationVideo = false;
	bool debug_detect_object = false;

	GPS::Location location;
	GPS::Location location2;

	#pragma pack(push)
	#pragma pack(1)
	struct ControlFrame
	{
		uint16_t Random;
		uint8_t IdFrame;
		uint8_t CamIndex;
		int16_t LeftSpeed;
		int16_t RightSpeed;
		uint32_t Command;
		uint32_t Value;
		int16_t ArmVerticalSpeed;
		int16_t ArmHorizontalSpeed;
		uint8_t PelcoD[7];
		uint8_t IdCameraView;
	};

	#pragma pack(pop)

	volatile bool navigating = false;

	GPS* gps;
	GPS* gps2;
	PowerUnit* power_unit;

	Solarbot();
	int ReadConf();
	void setNavigationMode(std::string s_mode);
	void setWayPointsFile(std::string s_way_points_file);
	void setCamNavigationAngleFactor(int v);
	void setNavigationCamera(std::string s_camera_path);
	void setShowVideo(int a_s);
	void setSaveVideo(int a_w, std::string save_video_path);
	int OpenHead();
	bool IsLocationGood();
	int run(volatile int* ctrl);
	int SendTelemetry(GPS::Location location, double heading, unsigned char gps2_quality);
	int InitModeWaypoint();
	int InitModeVisual();
	int InitModeLine();

private:
	volatile int* ctrl;
	std::string const ConfigurationFile = "/home/solarbot/solarbot.conf";	
	Telemetry* telemetry;
	Head* head;
	ControlUnit* control_unit;
	

	std::vector<Cam_Realsense*> realsense_cameras;
	std::vector<Detect_object*> object_detectors;
	std::vector<Cropmapping_Process*> cropmapping_processes;

	std::string UdpServerHost = "";
	uint16_t UdpServerPort;
	int UdpServerPacketSize = 512;
	int camnavigator_cam_index = -1;
	bool ModeWaypointInitialized = false;
	bool ModeVisualInitialized = false;
	bool ModeLineInitialized = false;

	//int CamWorkSpeed = 1000;

	void* ReadGPSThread();
	void* ReadGPS2Thread();
	double Degrees2Radians(double degree);
	double GetBearing(double from_lat, double from_lon, double to_lat, double to_lon);
	

	volatile unsigned char gps_working;
	pthread_t gps_thread;
	static void* gps_thread_helper(void* context)
	{
		return ((Solarbot*)context)->ReadGPSThread();
	}

	
	volatile unsigned char gps2_working;
	pthread_t gps2_thread;
	static void* gps2_thread_helper(void* context)
	{
		return ((Solarbot*)context)->ReadGPS2Thread();
	}

	
	void* ReadRTCMThread();
	pthread_t gps_rtcm_thread;
	static void* gps_rtcm_thread_helper(void* context)
	{
		return ((Solarbot*)context)->ReadRTCMThread();
	}
	int gps_rtcm_socketfd;
	struct sockaddr_in gps_rtcm_server_address;
	AES128 RTKServer_AES;
	unsigned char RTKServer_AES_Key[16];
	unsigned char RTKServerTxBuff[256];
	void SendAuthenticationToRTKServer();

	uint16_t last_arm_vertical_speed = 0;
	uint16_t last_arm_horizontal_speed = 0;
	PracticalSocket::UDPSocket rx_controller_sock;
	void* ReadRxControllerThread();
	pthread_t rx_controller_thread;
	static void* rx_controller_thread_helper(void* context)
	{
		return ((Solarbot*)context)->ReadRxControllerThread();
	}

	int rc_sock_fd;
	struct sockaddr_in rc_address;

	int gps_consumers_sock_fd;
	vector<GPSConsumer*> gps_consumers;

	uint32_t getbitu(char* buff, uint32_t pos, uint8_t len);
};

