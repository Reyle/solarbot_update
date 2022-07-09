#include <stdio.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>
#include <algorithm>
#include <chrono>
#include <boost/algorithm/string.hpp>

#include "GPS.h"
#include "Solarbot.h"
#include "err.h"
#include "UdpVideoSender.h"

//using namespace std;

int Solarbot::ReadConf()
{
	try
	{
		std::ifstream ifile(ConfigurationFile);
		if (ifile)
		{
			string line;
			while (std::getline(ifile, line))
			{
				if (line.length() > 0)
				{
					line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

					if (line.length() > 0)
					{
						if (line.c_str()[0] != '#')
						{
							if (line.substr(0, 21) == "ObjectDetectionCamera")
							{
								std::string cam_name = line.substr(21, line.length() - 21);
								boost::trim(cam_name);
								Detect_object* _detector = new Detect_object(cam_name, debug_detect_object);
								object_detectors.push_back(_detector);
							}
							else if (line.substr(0, 11) == "Cropmapping")
							{
								std::string mapping_name = line.substr(11, line.length() - 11);
								boost::trim(mapping_name);
								Cropmapping_Process* proc = new Cropmapping_Process(mapping_name);
								cropmapping_processes.push_back(proc);
							}
							else
							{
								std::istringstream is_line(line);
								std::string key;
								if (std::getline(is_line, key, '='))
								{
									std::string value;
									if (std::getline(is_line, value))
									{
										if (key == "MyID")
										{
											boost::trim(value);
											MyID = value;
											telemetry->ID = value;
										}
										else if (key == "Camera")
										{
											vector<string> strs;
											boost::split(strs, value, boost::is_any_of(","));
											if (strs.size() == 2)
											{
												boost::trim(strs[0]);
												boost::trim(strs[1]);
												if (strs[0].length() > 0 && strs[1].length() > 0)
												{
													Cam_Realsense* cam = new Cam_Realsense(strs[0], strs[1]);
													realsense_cameras.push_back(cam);
												}
											}
										}
										else if (key == "ROI")
										{
											Detect_object::Zone _zone;

											const char* p = value.c_str();
											int x = atoi(p);
											p = strchr(p, ',') + 1;
											int y = atoi(p);
											p = strchr(p, ',') + 1;
											int w = atoi(p);
											p = strchr(p, ',') + 1;
											int h = atoi(p);
											p = strchr(p, ',') + 1;
											int min_dist = atoi(p);
											p = strchr(p, ',') + 1;
											int max_dist = atoi(p);
											p = strchr(p, ',') + 1;
											int min_area = atoi(p);

											_zone.ROI = cv::Rect(x, y, w, h);
											_zone.min_distance = min_dist;
											_zone.max_distance = max_dist;
											_zone.min_area = min_area;

											object_detectors.back()->zones.push_back(_zone);
										}
										else if (key == "CROP")
										{
											cropmapping_processes.back()->parameters = value;
										}
										else if (key == "NavigationCamera")
										{
											NavigationCamera = value;
										}
										else if (key == "UdpServerHost")
										{
											UdpServerHost = value;
										}
										else if (key == "UdpServerPort")
										{
											UdpServerPort = atoi(value.c_str());
										}
										else if (key == "CamWorkSpeed")
										{
											navigator->CamWorkSpeed = atoi(value.c_str());
										}
										else if (key == "CamNavigation_angle_factor")
										{
											navigator->cam.navigation_angle_factor = atof(value.c_str());
										}
										else if (key == "CamNavigation_rows")
										{
											navigator->cam.rows = atoi(value.c_str());
										}
										else if (key == "CamNavigation_row_width")
										{
											navigator->cam.row_width = atoi(value.c_str());
										}
										else if (key == "CamNavigation_img_zoom")
										{
											navigator->cam.img_zoom = atof(value.c_str());
										}
										else if (key == "CamNavigation_perspective_diff")
										{
											navigator->cam.perspective_diff = atoi(value.c_str());
										}
										else if (key == "CamNavigation_horizontal_offset")
										{
											navigator->cam.horizontal_offset = atoi(value.c_str());
										}
										else if (key == "CamNavigation_vertical_offset")
										{
											navigator->cam.vertical_offset = atoi(value.c_str());
										}
										else if (key == "H_MIN")
										{
											navigator->cam.H_MIN = atoi(value.c_str());
										}
										else if (key == "H_MAX")
										{
											navigator->cam.H_MAX = atoi(value.c_str());
										}
										else if (key == "S_MIN")
										{
											navigator->cam.S_MIN = atoi(value.c_str());
										}
										else if (key == "S_MAX")
										{
											navigator->cam.S_MAX = atoi(value.c_str());
										}
										else if (key == "V_MIN")
										{
											navigator->cam.V_MIN = atoi(value.c_str());
										}
										else if (key == "V_MAX")
										{
											navigator->cam.V_MAX = atoi(value.c_str());
										}
										else if (key == "RTKServer_host")
										{
											RTKServer_host = value;
										}
										else if (key == "RTKServer_port")
										{
											RTKServer_port = (uint16_t)atoi(value.c_str());
										}
										else if (key == "RTKServer_password")
										{
											RTKServer_password = value;
										}
										else if (key == "RTKServer_asking_base")
										{
											RTKServer_asking_base = value;
										}
										else if (key == "Telemetry_localport")
										{
											telemetry->local_port = (uint16_t)atoi(value.c_str());
										}
										else if (key == "Telemetry_server_host")
										{
											telemetry->server_host = value;
										}
										else if (key == "Telemetry_server_port")
										{
											telemetry->server_port = (uint16_t)atoi(value.c_str());
										}
										else if (key == "Telemetry_server_user")
										{
											telemetry->server_user = value;
										}
										else if (key == "Telemetry_server_password")
										{
											telemetry->server_password = value;
										}
										else if (key == "CMD_version")
										{
											CMD_version = value;
										}
										else if (key == "CMD_user")
										{
											CMD_user = value;
										}
										else if (key == "CMD_password")
										{
											CMD_password = value;
										}
										else if (key == "GPSConsumer")
										{
											vector<string> strs;
											boost::split(strs, value, boost::is_any_of(":"));
											if (strs.size() == 2)
											{
												boost::trim(strs[0]);
												boost::trim(strs[1]);
												if (strs[0].length() > 0 && strs[1].length() > 0)
												{
													//struct sockaddr_in address;
													//memset(&address, 0, sizeof(address));
													//address.sin_family = AF_INET; // IPv4 
													//inet_pton(AF_INET, strs[0].c_str(), &(address.sin_addr));//Convert string to IP
													//address.sin_port = htons((uint16_t)atoi(strs[1].c_str()));
													//gps_consumers_addresss.push_back(address);		

													GPSConsumer* gps_consumer = new GPSConsumer(strs[0], strs[1]);
													gps_consumers.push_back(gps_consumer);

												}
											}
										}
										//telemetry->Start("SolarbotUsa", 2010, "52.161.96.125", 3000, "usuario", "contrasena");
									}
									//	store_line(key, value);
								}
							}
						}
					}
				}
			}
			ifile.close();

			std::cout << std::endl << "Object Detectors:" << std::endl;
			for (auto obj : object_detectors)
			{
				std::cout << "Camera: " << obj->camera_name << std::endl;
				for (auto z : obj->zones)
				{
					std::cout << "\tROI: " << z.ROI.x << ", " << z.ROI.y << ", " << z.ROI.width << ", " << z.ROI.height << ", " << z.min_distance << ", " << z.max_distance << std::endl;
				}
			}
			std::cout << std::endl;

			std::cout << std::endl;
			std::cout << "Udp Video Server: " << UdpServerHost << ":" << UdpServerPort << " packet size:" << UdpServerPacketSize << std::endl;

		}
		else
		{
			cout << ConfigurationFile << " was not found." << endl;
		}
	}
	catch (...)
	{

	}

	return 0;
}

void* Solarbot::ReadGPSThread()
{
	while (gps_working)
	{
		gps->getLocation(&location); //Get GPS position
	}
	return nullptr;
}

void* Solarbot::ReadGPS2Thread()
{
	while (gps2_working)
	{
		gps2->getLocation(&location2); //Get GPS position
	}
	return nullptr;
}

Solarbot::Solarbot()
{
	navigator = new Navigator();
	telemetry = new Telemetry();
	location.latitude = 0.0;
	location.longitude = 0.0;
}

void Solarbot::setNavigationMode(std::string s_mode)
{
	navigator->Mode = navigator->String2Mode(s_mode);
}

void Solarbot::setWayPointsFile(std::string s_way_points_file)
{
	WaypointsFile = s_way_points_file;
}

void Solarbot::setCamNavigationAngleFactor(int v)
{
	navigator->cam.navigation_angle_factor = v;
}

void Solarbot::setNavigationCamera(std::string s_camera_path)
{
	NavigationCamera = s_camera_path;
}

void Solarbot::setShowVideo(int a_s)
{
	if (a_s == 1)
	{
		navigator->cam.DEBUG_CAM_NAVIGATOR = true;
		navigator->cam.SHOW_CAM_NAVIGATOR = true;
	}
}

void Solarbot::setSaveVideo(int a_v, std::string save_video_path)
{
	if (a_v == 1)
	{
		navigator->cam.DEBUG_CAM_NAVIGATOR = true;
		navigator->cam.SAVE_CAM_NAVIGATOR = true;
		navigator->cam.save_video_filename = save_video_path;
	}
}

int Solarbot::OpenHead()
{
	if (head->Open() < 0)
	{
		return err::ERR_OPENING_HEAD;
	}

	int i = 0;
	int r = head->ReadSensors();
	while (r != 0 && i < 10)
	{
		r = head->ReadSensors();
		usleep(10000);
		i++;
	}

	if (i >= 10)
	{
		std::cout << "ERROR reading head sensors..." << std::endl;
		return err::ERR_OPENING_HEAD;
	}
	//else
	//{
	//	std::cout << "Reading head sensors start errors count=" << i << std::endl;

	//}

	return 0;
}

int Solarbot::run(volatile int* ctrl)
{
	this->ctrl = ctrl;

	int r = 0;

	//Try to kill cropmapping.out process 
	try
	{
		boost::process::spawn("killall", "cropmapping.out");
	}
	catch (...) {}

#pragma region Setting and create socket to send messages to RemoteControler computer
	memset(&rc_address, 0, sizeof(rc_address));
	rc_address.sin_family = AF_INET; // IPv4 
	inet_pton(AF_INET, "172.16.1.60", &(rc_address.sin_addr));//Convert string to IP
	rc_address.sin_port = htons(2001);

	// Creating socket Main file descriptor 
	if ((rc_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket RC creation failed");
		return -1;
	}
#pragma endregion 

#pragma region Setting and create socket to send messages to GPS Consumers
	// Creating socket Main file descriptor 
	if ((gps_consumers_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket GPS Consumers creation failed");
		return -1;
	}
#pragma endregion 
	
#pragma region Open Head
	head = new Head();
	printf("Opening Head sensors...");
	r = OpenHead();
	if (r != 0)
	{
		printf("ERROR\nERROR: opening Head sensors. code=%d\n", r);
		delete head;
		return r;
	}
	head->SetEyes(Head::E_LOADING);

	if (IsBrasil)
		head->SetMagneticDeclination(-20.17); //ROBOT BRASIL 
	else
		head->SetMagneticDeclination(-4.3); //ROBOT West Lafayette 

	printf("OK\n");
#pragma endregion 

#pragma region Open GPS
	printf("Opening GPS1...");	
	std::ifstream ifile("/dev/ttyGPS");
	if ((bool)ifile)
	{	
		gps = new GPS();
		r = gps->Open("/dev/ttyGPS", 115200);
		if (r !=0)
		{
			printf("Error opening GPS1, code=%d\n", r);
			delete gps;
			head->Close();
			delete head;
			return r;
		}
		else
		{
			printf("OK\n");
		}
	}
	else
	{
		printf("Error GPS1 not found!\n");
		head->Close();
		delete head;
		return err::ERR_NOT_FOUND;
	}
#pragma endregion 

#pragma region Open GPS2
	printf("Opening GPS2...");	
	std::ifstream ifile_gps2("/dev/ttyGPS2");
	if ((bool)ifile_gps2)
	{		
		gps2 = new GPS();
		r = gps2->Open("/dev/ttyGPS2", 115200);
		if (r != 0)
		{
			printf("Error opening GPS2, code=%d\n", r);
			gps2->Close();
			delete gps2;
			gps->Close();
			delete gps;
			head->Close();
			delete head;
			return r;
		}
		else
		{
			printf("OK\n");
		}
	}
	else
	{
		printf("Error GPS2 not found!\n");
		gps->Close();
		delete gps;
		head->Close();
		delete head;
		return -1;
	}
#pragma endregion 

#pragma region Start RTCM Received Thread
	if (pthread_create(&gps_rtcm_thread, NULL, &Solarbot::gps_rtcm_thread_helper, this))
	{
		printf("Error starting RTCM Receive Thread not found!\n");
		gps->Close();
		delete gps;
		head->Close();
		delete head;
		return -1;
	}
#pragma endregion

#pragma region Open_Control_Unit
	control_unit = new ControlUnit();
	int driver_status;
	printf("Opening Drivers...");	
	r = control_unit->Open();
	if (r !=0 )
	{
		printf("Error opening Driver, code=%d\n", r);
		gps2->Close();
		delete gps2;
		gps->Close();
		delete gps;
		head->Close();
		delete head;		
		return r;
	}
	else
	{
		printf("OK\n");
		driver_status = control_unit->Write(0, 0); //Write to driver to be sure is stop and read the status
		printf("Driver first read result code=%d\n", driver_status);
	}
#pragma endregion

#pragma region Open_Power_Unit
	printf("Opening Power Unit...");
	power_unit = new PowerUnit();
	int power_unit_status;
	r = power_unit->Open("/dev/ttyPowerUnit", 115200);
	if (r != 0)
	{
		printf("Error opening Power Unit, code=%d\n", r);
		control_unit->Close();
		delete control_unit;
		gps2->Close();
		delete gps2;
		gps->Close();
		delete gps;
		head->Close();
		delete head;
		return r;
	}
	else
	{
		printf("OK\n");
		power_unit_status = power_unit->Read(); //Get power unit data
		printf("Power Unit first read result code=%d\n", power_unit_status);
	}
#pragma endregion
	
#pragma region Create_rx_controller_thread
	printf("Creating remote controller server...");
	if (pthread_create(&rx_controller_thread, NULL, &Solarbot::rx_controller_thread_helper, this) != 0)
	{
		printf("ERROR: rx_controller_thread creation fails\n");
		
		power_unit->Close();
		delete power_unit;
		control_unit->Close();
		delete control_unit;
		gps2->Close();
		delete gps2;
		gps->Close();
		delete gps;
		head->Close();
		delete head;
		return -1;
	}
	else
	{
		printf("OK\n");
	}
#pragma endregion

#pragma region Calibrate_Magnetometer
	//while (1)
	//{
	//	std::cout << "Heading: " << hmc5883L->getHeading() << endl;
	//	sleep(1);
	//}

	//std::cout << "Calibrating Magnetometer..." << std::endl;
	////Para Aracatuba la declinacion magnetica es 20.08W
	//hmc5883L->CalibrateHeading(3000, 0.0);
	//printf("X=%d\tY=%d\tZ=%d\r\n", hmc5883L->x_offset, hmc5883L->y_offset, hmc5883L->z_offset);
	//return 0;
#pragma endregion

#pragma region Init Navigator
	navigator->Init(control_unit, head);
	printf("Navigation inizialized\n");
#pragma endregion

#pragma region Init Telemetry
	//telemetry = new Telemetry();
	//telemetry->Start("SolarbotUsa", 2010, "52.161.96.125", 3000, "usuario", "contrasena");
	telemetry->Start();
	printf("Telemetry inizialized\n");	
#pragma endregion

#pragma region Start the realsense cameras 
	printf("Starting RealSense cameras...\n");
	for (Cam_Realsense* cam : realsense_cameras)
	{
		//Look for if someone use this camera

		if (cam->Open() < 0)
		{
			printf("ERROR starting camera %s (%s)\n", cam->name, cam->id_cam);
			*ctrl = 0; //to exit
			break;
		}
		else
		{
			cam->InUse = true;
			
			//cam->rgb_sensor.set_option(rs2_option::RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, false);
			////cam->rgb_sensor.set_option(rs2_option::RS2_OPTION_EXPOSURE, 6);
			//cam->rgb_sensor.set_option(rs2_option::RS2_OPTION_WHITE_BALANCE, 4909);

			//cam->rgb_sensor.set_option(rs2_option::RS2_OPTION_SATURATION, 90);
			//cam->rgb_sensor.set_option(rs2_option::RS2_OPTION_GAMMA, 500);
			//cam->rgb_sensor.set_option(rs2_option::RS2_OPTION_ENABLE_AUTO_EXPOSURE, true);

			printf("Started camera %s (%s)\n", cam->name.c_str(), cam->id_cam.c_str());
		}
	}
#pragma endregion 

#pragma region Look for camera id used in Object Detectors
	for (Detect_object* detector : object_detectors)
	{
		for (int i=0; i< realsense_cameras.size(); i++)
		{
			if (realsense_cameras[i]->name == detector->camera_name)
			{
				detector->camera_id = i;
				break;
			}
		}
	}	
#pragma endregion 

#pragma region Start cropmapping process
	for (Cropmapping_Process* proc : cropmapping_processes)
	{
		//Search the camera index
		for (int i = 0; i < realsense_cameras.size(); i++)
		{
			if (proc->name == realsense_cameras[i]->name)
			{
				proc->camera_index = i;
				if (proc->Start(3 * realsense_cameras[i]->width_bgr * realsense_cameras[i]->height_bgr) == 0)
					std::cout << "Cropmapping Process \"" << proc->name << "\" started." << std::endl;
				else
					std::cerr << "ERROR: Starting Cropmapping Process \"" << proc->name << "\"" << std::endl;
				break;
			}
		}
	}
#pragma endregion 

	double compass_heading = -1; //True north heading corrected from magnetic heading
	double gps_heading = -1; //Hight precision heading from the two GPSs compare positions
	double heading = -1;
	char buffer[256];
	int toTxTelemetry = 4;
	

#pragma region Init Mode	
	int init_result = 0;
	if (navigator->Mode == Navigator::Modes::ModeVisual)
	{
		init_result = InitModeVisual();
	}
	else if (navigator->Mode == Navigator::Modes::ModeLine)
	{
		init_result = InitModeLine();
	}
	else if (navigator->Mode == Navigator::Modes::ModeWaypoints)
	{
		init_result = InitModeWaypoint();
	}

	if (init_result == 0)
	{
		//Set navigation if init was OK and mode is different than standby
		navigating = navigator->Mode != Navigator::Modes::Standby;
	}
	else
	{
		//We have problem init something necessary, so quit
		return init_result;
	}
#pragma endregion 

	head->SetEyes(Head::E_NEUTRAL);
	//head->SetEyes(Head::E_SAD, true, true);
	int eye_blink_count = 0;

	double last_object_detection_stop = clock();
	double last_object_detection_stop_filter = clock() - (5 * CLOCKS_PER_SEC);
	double object_detection_time_to_stop = CLOCKS_PER_SEC / 2;
	double object_detection_time_to_start = 3 * CLOCKS_PER_SEC;
	int critical_object_detected_count;

	bool look_left = false;
	bool look_right = false;
	int where_to_look = 0;
	int last_where_to_look = -2;

	//int img_navigation_cam_width = 424;
	//int img_navigation_cam_height = 240;
	////int img_navigation_frame_zone_offset = 280;
	//int img_navigation_frame_zone_width = 350;
	//int img_navigation_frame_width = 2 * img_navigation_frame_zone_width;
	//cv::Mat img_navigation_frame(cv::Size(img_navigation_frame_width, img_navigation_cam_height), CV_8UC3);
	//Mat img_navigation_roi_left = img_navigation_frame(Rect(0, 0, img_navigation_frame_zone_width, img_navigation_cam_height));
	//Mat img_navigation_roi_right = img_navigation_frame(Rect(img_navigation_frame_zone_width, 0, img_navigation_frame_zone_width, img_navigation_cam_height));

	//UdpVideoSender video_sender(UdpServerHost, UdpServerPort, UdpServerPacketSize);
	//Mat img_navigation_frame;
	//VideoCapture cap_remote_navigation;
	//if (TransmitNavigationVideo)
	//{
	//	TransmitNavigationVideo = cap_remote_navigation.open("/dev/videoRemote", CAP_V4L2);
	//}

	
	CamNavigator::Result cam_result;

	std::chrono::_V2::system_clock::time_point last_cycle = std::chrono::high_resolution_clock::now();
	std::chrono::_V2::system_clock::time_point now_cycle;
	char location_text[64];

#pragma region Main_Loop
	while (*ctrl == 1)
	{
#pragma region Read GPSs
		//Get GPS position
		//Each 250ms a gps sends a fix
		gps->getLocation(&location);
		gps2->getLocation(&location2);

		//Send location to RemoteController computer
		int location_text_len = sprintf(location_text, "%.8f, %.8f", location.latitude, location.longitude);
		sendto(rc_sock_fd, location_text, location_text_len, MSG_CONFIRM, (const struct sockaddr*)&rc_address, sizeof(rc_address));

		now_cycle = std::chrono::high_resolution_clock::now();
		int cycle_duration = (std::chrono::duration_cast<std::chrono::milliseconds>(now_cycle - last_cycle).count());
		//std::cout << cycle_duration << "ms" << std::endl;
		last_cycle = now_cycle;
					
		if (IsLocationGood())
		{
			gps_heading = GetBearing(location2.latitude, location2.longitude, location.latitude, location.longitude);
			location.course = gps_heading;
			location.status = 'R'; //Deulis: Means that the course was calculated by two antennas
		}
		else
		{
			cerr << "GPSs poor quality Q1=" << (int)location.quality << " Q2=" << (int)location2.quality << endl;
			gps_heading = -1;
		}

		//Send location to GPS Consumers
		try
		{
			int c = gps_consumers.size();
			unsigned char* p = (unsigned char*)&location;
			int length = sizeof(location);
			int address_len;
			for (int i = 0; i < c; i++)
			{
				address_len = sizeof(gps_consumers[i]->address);
				sendto(gps_consumers_sock_fd, p, length, MSG_CONFIRM, (const struct sockaddr*)&gps_consumers[i]->address, address_len);
			}
		}
		catch (...) {}

#pragma endregion

#pragma region Read Head Sensors
		r = head->ReadSensors();
		if (r == 0)
		{
			compass_heading = head->data.Heading; //Get magnetic heading
		}
		else
		{
			compass_heading = -1;
			printf("ERROR: Reading Head Sensors, code=%d\n", r);

			if (reopen_head_on_error)
			{
				head->Close();
				r = OpenHead();
				if (r != 0)
				{
					printf("ERROR: Retrying open Head Sensors, code=%d\n", r);
				}
			}
		}
#pragma endregion

#pragma region Select best heading
		if (gps_heading > -1)
			heading = gps_heading;
		else
			heading = compass_heading;
#pragma endregion

#pragma region Read cameras
		for (Cam_Realsense* cam : realsense_cameras)
		{
			if (cam->InUse)
			{
				cam->Read();
				//cv::imshow(cam->name, cam->mat_bgr);
				//cv::waitKey(10);
			}
		}
#pragma endregion

		////BORRAR!!!!!
		//for (Cropmapping_Process* proc : cropmapping_processes)
		//{
		//	proc->Share(location.latitude, location.longitude, gps_heading, realsense_cameras[proc->camera_index]->mat_bgr);
		//}

#pragma region DETECT OBJECT
		critical_object_detected_count = 0;
		bool look_left = false;
		bool look_right = false;
		where_to_look = 0;

		for(Detect_object* detector : object_detectors)
		{
			std::vector<uint16_t> results = detector->search(&realsense_cameras[detector->camera_id]->mat_depth, &realsense_cameras[detector->camera_id]->mat_bgr);

			int results_count = results.size();
			for (int r = 0; r < results_count; r++)
			{
				if (results[r] != UINT16_MAX)
				{
#ifdef DEBUG_DETECT_OBJECT
					cout << detector->camera_name << ":" << r + 1 << "= " << results[r] << endl;
#endif
					critical_object_detected_count++;

					if (detector->camera_name == "Left")
						look_left = true;

					if (detector->camera_name == "Right")
						look_right = true;

					break;
				}
			}
		}

		if (critical_object_detected_count > 0)
		{
			if (clock() - last_object_detection_stop_filter > object_detection_time_to_stop)
			{
				std::cout << "STOP! STOP! STOP! STOP! STOP! STOP! " << endl;
				navigator->DoNotMove = true;
				last_object_detection_stop = clock();

			}
		}
		else
		{
			last_object_detection_stop_filter = clock();

			if (navigator->DoNotMove && clock() - last_object_detection_stop > object_detection_time_to_start)
			{
				navigator->DoNotMove = false;
				cout << "MOVE " << endl;
			}
		}
#pragma endregion

#pragma region Navigating
		if (navigating)
		{
			
			if (navigator->Mode == Navigator::Modes::ModeWaypoints)
			{
				if (!IsLocationGood())
				{
					//printf("GPS not ready. code=%d\n", location.quality);
					head->SetEyes(Head::E_X);
				}
				else
				{
					double remaining_distance = 0.0;
					r = navigator->GotoNextWaypoint(location, gps_heading, &remaining_distance);

					if (r == err::NAVIGATOR_DONE)
					{
						navigating = false;
						navigator->Mode == Navigator::Modes::Standby;
					}

					printf("waypoint=%d\tdist.=%0.2f\tGPS=%d,%d", navigator->waypoints_index, remaining_distance, location.quality, location2.quality);
					if (navigator->finding_path)
						printf(" (finding path)\n");
					else
						printf("\n");

					//Send location and image to cropmapping processes
					for (Cropmapping_Process* proc : cropmapping_processes)
					{
						proc->Share(location.latitude, location.longitude, gps_heading, realsense_cameras[proc->camera_index]->mat_bgr);
					}
				}
			}
			else if (navigator->Mode == Navigator::Modes::ModeVisual)
			{
				cam_result = navigator->GotoNextCam(realsense_cameras[camnavigator_cam_index]->mat_bgr);
				//printf("CAM a=%.1f\tq=%d\tr=%d\tL=%d\tR=%d\tPanel=%.1fV %dW\tBatt=%.1fV %dW\n", cam_result.angle, cam_result.quality, cam_result.used_row, navigator->MotorLeft, navigator->MotorRight, power_unit->data.PanelVoltage, power_unit->data.PanelPower, power_unit->data.BatteryVoltage, power_unit->data.BatteryPower);
				//Send location and image to cropmapping processes
				for (Cropmapping_Process* proc : cropmapping_processes)
				{
					proc->Share(location.latitude, location.longitude, heading, realsense_cameras[proc->camera_index]->mat_bgr);
				}
			
			}
			else if (navigator->Mode == Navigator::Modes::ModeLine)
			{
				r = navigator->GotoByLine(); //Navigate by line Sensor
				printf("Left=(%d)%.1f\tRight=(%d)%.1f\tAngle=%.1f\tLcmd=%d\tRcmd=%d\n", navigator->line_data.raw_left, navigator->line_data.Left, navigator->line_data.raw_right, navigator->line_data.Right, navigator->line_data.angle, navigator->MotorLeft, navigator->MotorRight);

				//Send location and image to cropmapping processes
				for (Cropmapping_Process* proc : cropmapping_processes)
				{
					proc->Share(location.latitude, location.longitude, heading, realsense_cameras[proc->camera_index]->mat_bgr);
				}
			}
		}
		else
		{

		}

#pragma endregion

#pragma region Telemetry
		if (toTxTelemetry > 2)
		{
			toTxTelemetry = 0;

			//Telemetry Power Unit Data
			power_unit_status = power_unit->Read(); //Get power unit data
			if (power_unit_status != 0)
			{
				printf("ERROR reading power unit, code %d\n", power_unit_status);
				printf("RESETING POWER UNIT CONNECTION...");
				power_unit->Close();
				power_unit->Open();
				power_unit_status = power_unit->Read(); //Get power unit data second reading
			}

			driver_status = control_unit->Read();
			if (driver_status != 0)
			{
				printf("ERROR reading Control Unit, code %d\n", driver_status);
				printf("RESETING DRIVER CONNECTION...");
				control_unit->Close();
				control_unit->Open();
				driver_status = control_unit->Read(); //Get power unit data second reading
			}


			if (power_unit_status == 0 && driver_status == 0)
			{				
				SendTelemetry(location, compass_heading, location2.quality);
			}



			if (!navigating && heading > -1)
			{
				if (look_left || look_right)
				{
					eye_blink_count = 0;
					if (look_left && look_right)
					{
						where_to_look = 0;
					}
					else
					{
						if (look_left)
							where_to_look = -1;
						else
							where_to_look = 1;

					}

					if (where_to_look != last_where_to_look)
					{
						last_where_to_look = where_to_look;

						if (look_left && look_right)
							head->SetEyes(Head::E_WINK, false, true, false);
						else if (look_left)
							head->SetEyes(Head::E_LOOK_L, false, true, false);
						else
							head->SetEyes(Head::E_LOOK_R, false, true, false);
					}
				}
				else
				{
					last_where_to_look = -2;
					eye_blink_count++;
					if (eye_blink_count > 5)
					{
						eye_blink_count = 0;
						head->SetEyes(Head::E_BLINK, true, false, true);
					}
					else
					{
						head->SetEyes(Head::E_NEUTRAL);
					}
				}
			}
		}
		toTxTelemetry++;
#pragma endregion
	}
#pragma endregion 

#pragma region Termitate
	printf("\nTerminating...\n");

	for (Cam_Realsense* c : realsense_cameras)
	{
		try
		{
			if (c->InUse)
				c->Close();
		}
		catch (...) {}
	}

	if (navigator->cam.SAVE_CAM_NAVIGATOR)
	{
		//navigator->cam.video_writer.release();
	}


	std::cout << "Deleting GPS" << std::endl;
	delete gps;
	gps->Close();

	std::cout << "Deleting GPS2" << std::endl;
	delete gps2;
	gps2->Close();

	std::cout << "Deleting Power Unit" << std::endl;
	power_unit->Close();
	delete power_unit;

	//std::cout << "Deleting Navigator" << std::endl;
	//delete navigator;

	std::cout << "Deleting Control Unit" << std::endl;
	control_unit->Close();
	delete control_unit;

	std::cout << "Deleting Telemetry" << std::endl;
	delete telemetry;

	for(int i=0; i < gps_consumers.size(); i++)
		delete gps_consumers[i];
	gps_consumers.clear();

	head->SetEyes(Head::E_SLEEP, false, true);
	sleep(1);
	head->Close();
	delete head;

	printf("\nSolarbot Terminated.\n");

	return 0;

#pragma endregion 
}

int Solarbot::SendTelemetry(GPS::Location location, double heading, unsigned char gps2_quality)
{
	Telemetry::Frame frame;

	frame.FrameValid1 = 0x73;
	frame.FrameValid2 = 0x02;

	//Telemetry GPS 
	frame.GPSLatitude = (int)(10000000 * location.latitude);
	frame.GPSLongitude = (int)(10000000 * location.longitude);
	frame.GPSAltitude = (int)(10 * location.altitude);
	frame.GPSSpeed = (int)(10 * location.speed);
	frame.GPSBearing = (int)(10 * location.course);
	frame.GPSStatus = (10*location.quality) + gps2_quality;
	frame.GPSBaseID = (unsigned short)location.StationID;

	//Telemetry Sensors
	frame.MagneticBearing = (short)(10 * heading);
	frame.PitchAngle = 0;
	frame.RollAngle = 0;

	//Telemetry Power Sensors Data
	frame.PowerSensorsStatus = power_unit->data.Status;
	if (power_unit->data.Status == 0)
	{
		frame.PowerSensorsIdFrame = power_unit->data.IdFrame;
		frame.PanelVoltage = (short)(10 * power_unit->data.PanelVoltage);
		frame.BatteryVoltage = (short)(10 * power_unit->data.BatteryVoltage);
		frame.PanelPower = power_unit->data.PanelPower;
		frame.BatteryPower = power_unit->data.BatteryPower;
		frame.ElectronicPower = power_unit->data.ElectronicPower;
		frame.LeftPower = power_unit->data.LeftPower;
		frame.RightPower = power_unit->data.RightPower;
		frame.StopPressed = power_unit->data.StopPressed;
	}

	if (control_unit->data.Status == 0)
	{
		frame.LeftMotorSpeed = control_unit->data.LeftSpeed * 100;
		frame.RightMotorSpeed = control_unit->data.RightSpeed * 100;
		frame.LeftAlarm = control_unit->data.LeftAlarm;
		frame.RightAlarm = control_unit->data.RightAlarm;
		frame.RemoteControlled = control_unit->data.RemoteControlled;
		//cout << "L=" << driver->data.LeftSpeed << " R=" << driver->data.RightSpeed << endl;
	}


	//Telemetry Navigation
	frame.LeftMotorCommand = (short)(navigator->MotorLeft);
	frame.RightMotorCommand = (short)(navigator->MotorRight);
	frame.CurrentDestinationPointIndex = navigator->waypoints_index;
	frame.CurrentDestinationPointLatitude = (int)(10000000 * (navigator->GetCurrentWaypoint().latitude));
	frame.CurrentDestinationPointLongitude = (int)(10000000 * (navigator->GetCurrentWaypoint().longitude));
	frame.CurrentDestinationPointSpeed = (int)(10 * (navigator->GetCurrentWaypoint().speed));

	frame.FrameID = telemetry->FrameID++;

	//Send Telemetry
	telemetry->SendFrame(1, (unsigned char*)&frame, sizeof(frame));

	return 0;
}

int Solarbot::InitModeWaypoint()
{
	if (ModeWaypointInitialized)
		return 0;

	if (WaypointsFile != "")
	{
		int r = navigator->LoadWaypoints(WaypointsFile.c_str());
		if (r != 0)
		{
			printf("ERROR: opening Way Points File, code=%d\n", r);
			return r;
		}
		else
		{
			printf("Loaded %d waypoints, start waypoint=", navigator->waypoints_size);
			if (navigator->start_waypoint_index == -1)
				printf("(look for closest point)\n");
			else
				printf("%d\n", navigator->start_waypoint_index);
		}

		ModeWaypointInitialized = true;
		return 0;
	}
	else
	{
		printf("ERROR: There is not any Way Points File\n");
		return err::err_codes::ERR_NOT_FOUND;
	}
}

int Solarbot::InitModeVisual()
{
	if (ModeVisualInitialized)
		return 0;

	if (!navigator->cam.Init(424, 240))
	{
		cout << "ERROR: Initializing CamNavigator" << endl;
		return err::ERR_OPENING_NAVIGATION_VISUAL;
	}

	//Look for the camera index used for navigation
	bool found_camera = false;
	for (int i = 0; i < realsense_cameras.size(); i++)
	{
		if (realsense_cameras[i]->name == NavigationCamera)
		{
			camnavigator_cam_index = i;
			found_camera = true;			
			break;
		}
	}

	if (found_camera)
	{
		ModeVisualInitialized = true;
		return 0;
	}
	else
	{
		cout << "ERROR: Navigation Camera Not Found" << endl;
		return err::err_codes::ERR_NOT_FOUND;
	}
}

int Solarbot::InitModeLine()
{
	if (ModeLineInitialized)
		return 0;

	int r = navigator->line.Init(LineSensorPath, 9600, 0, 0, 1.0, 1.0, 1.0);
	if (r)
	{
		cout << "ERROR: Openning Line Navigator, Code=" << r << endl;
		return err::ERR_OPENING_NAVIGATION_LINE;
	}
	else
	{
		ModeLineInitialized = true;
		return 0;
	}
}

double Solarbot::Degrees2Radians(double degree)
{
	return degree * M_PI / 180;
}

double Solarbot::GetBearing(double from_lat, double from_lon, double to_lat, double to_lon)
{
	double lat1 = Degrees2Radians(from_lat); //a
	double long1 = Degrees2Radians(from_lon); //b
	double lat2 = Degrees2Radians(to_lat); //c
	double long2 = Degrees2Radians(to_lon); //d

	if (cos(lat2) * sin(long2 - long1) == 0)
		if (lat2 > lat1)
			return 0;
		else
			return 180;
	else
	{
		double angle = atan2(cos(lat2) * sin(long2 - long1), sin(lat2) * cos(lat1) - sin(lat1) * cos(lat2) * cos(long2 - long1));
		return ((int)(angle * 180 / M_PI) + 360) % 360;
	}
}

bool Solarbot::IsLocationGood()
{
	return location.quality == 4 && location2.quality == 4;
}

uint32_t Solarbot::getbitu(char* buff, uint32_t pos, uint8_t len) {
	uint32_t bits = 0;

	for (uint32_t i = pos; i < pos + len; i++) {
		bits = (bits << 1) + ((buff[i / 8] >> (7 - i % 8)) & 1u);
	}

	return bits;
}

void* Solarbot::ReadRTCMThread()
{

	printf("RTK Server %s:%u\n", RTKServer_host.c_str(), RTKServer_port);

	const int MAX_UDP_SIZE = 1024;
	char gps_rtcm_rx_buffer[MAX_UDP_SIZE];

	// Created and Fill server address information
	memset(&gps_rtcm_server_address, 0, sizeof(gps_rtcm_server_address));
	gps_rtcm_server_address.sin_family = AF_INET; // IPv4 
	inet_pton(AF_INET, RTKServer_host.c_str(), &(gps_rtcm_server_address.sin_addr));//Convert string to IP
	gps_rtcm_server_address.sin_port = htons(RTKServer_port);

	// Creating socket A file descriptor	
	if ((gps_rtcm_socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("ERROR: Socket for GPS RTCM Receiver creation failed");
		return nullptr;
	}

	//Set Socket timeout to 10ms
	struct timeval read_timeout;
	read_timeout.tv_sec = 2;
	read_timeout.tv_usec = 0;
	setsockopt(gps_rtcm_socketfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

	//Generate any fake key
	for (int i = 0; i < 16; i++)
		RTKServer_AES_Key[0] = 0x45 + i;
	RTKServer_AES.SetKey(RTKServer_AES_Key);

	//Send First "anything" to the server just to the server request for authentication
	SendAuthenticationToRTKServer();

	int n = 0;
	struct sockaddr_in rx_address;
	int rx_address_len = sizeof(rx_address);
	memset(&rx_address, 0, rx_address_len);
	int tick_to_send_aloha = 0;
	int RTCM_count = 0;
	while (*ctrl == 1)
	{
		//printf("AWAITING RTCMC*********\n");
		rx_address_len = sizeof(rx_address); //len is value/result
		n = recvfrom(gps_rtcm_socketfd, gps_rtcm_rx_buffer, MAX_UDP_SIZE, MSG_WAITALL, (struct sockaddr*)&rx_address, (socklen_t*)&rx_address_len);
		//printf("RX RTCMC*********[%d]\n", n);
		if (n > 4)
		{
			//printf("RX: ");
			//for (int i = 0; i < n; i++)
			//	printf("%02X ", gps_rtcm_rx_buffer[i]);
			//printf("\n");

			if (gps_rtcm_rx_buffer[0] == 0xD3)
			{
				//printf("RX RTCM\n");
				RTCM_count++;
				//int type = getbitu(gps_rtcm_rx_buffer, 24, 12);
				//printf("Rx RTCM %d [%d] len=%d\n", RTCM_count, type, n);
				
				gps->Write(gps_rtcm_rx_buffer, n);
				gps2->Write(gps_rtcm_rx_buffer, n);

				//printf("Rx RTCM3 [%d] fd=%d\n", n, gps_rtcm_socketfd);
			}
			else if (gps_rtcm_rx_buffer[0] == 'R' && gps_rtcm_rx_buffer[1] == 'O' && gps_rtcm_rx_buffer[2] == 'V')//Is asking for authentication?
			{
				//Extract the AES key
				int sv = 11;
				for (int i = 0; i < 16; i++)
				{
					RTKServer_AES_Key[i] = gps_rtcm_rx_buffer[sv + 4];
					sv = gps_rtcm_rx_buffer[sv + 4] / 4;
				}

				printf("key=");
				for (int i = 0; i < 16; i++)
					printf("%02X ", RTKServer_AES_Key[i]);
				printf("\n");

				RTKServer_AES.SetKey(RTKServer_AES_Key);
				SendAuthenticationToRTKServer();
				printf("Sent authentication to RTK Server (Asked by server) [%d] fd=%d\n", n, gps_rtcm_socketfd);
			}
			else
			{
				printf("ERROR: Rx RTCM UNKNOWN FRAME [%d]\n", n);
			}
		}
		else
		{
			tick_to_send_aloha++;
			//printf("ERROR: Rx RTCM timeout [%d]\n", n);

			if (tick_to_send_aloha > 4)
			{
				tick_to_send_aloha = 0;
				SendAuthenticationToRTKServer();
				printf("Sent authentication to RTK Server (Rx less than 4 bytes) [%d] fd=%d\n", n, gps_rtcm_socketfd);
			}
		}
	}

	return nullptr;
}

void Solarbot::SendAuthenticationToRTKServer()
{

	//Frame: ROV,ID,Password,AskBase,Latitude,Longitude
	unsigned char buff[128];
	int c = sprintf((char*)buff, "ROV,%s,%s,%s,%0.6f,%0.6f", MyID.c_str(), RTKServer_password.c_str(), RTKServer_asking_base.c_str(), location.latitude, location.longitude);

	//Padding with zeros at the end
	int b = 1 + (c / 16); //Number of necessary blocks
	int r = (16 * b) - c; //Get how many left to fill the last block
	while (r > 0)//Complete at the end with 0x00
	{
		buff[c++] = 0x00;
		r--;
	}

	//Encryt the message
	RTKServer_AES.Encrypt(buff, buff, b);

	//char peer_addr_str[INET_ADDRSTRLEN];	
	//inet_ntop(AF_INET, &gps_rtcm_server_address, peer_addr_str, INET_ADDRSTRLEN);
	//printf("Send RTK Authentication to: %s port:%u\n", peer_addr_str, ntohs(gps_rtcm_server_address.sin_port));

	printf("Send RTK Authentication to: %s port:%u fd=%d\n", inet_ntoa(gps_rtcm_server_address.sin_addr), ntohs(gps_rtcm_server_address.sin_port), gps_rtcm_socketfd);

	

	//Finally send the frame to the RTKServer
	sendto(gps_rtcm_socketfd, buff, c, MSG_CONFIRM, (const struct sockaddr*)&gps_rtcm_server_address, sizeof(gps_rtcm_server_address));

	

}

void* Solarbot::ReadRxControllerThread()
{
	try
	{
		rx_controller_sock.setLocalPort(60000);

		char buff[32];
		while (true)
		{
			rx_controller_sock.recv(buff, 32);
			struct ControlFrame* rx_ControlFrame = (Solarbot::ControlFrame*)buff;

			if (rx_ControlFrame->Command == 0x7302)
			{
				if (rx_ControlFrame->Value == 1)
				{
					std::cout << "CONTROLLER Left=" << rx_ControlFrame->LeftSpeed << "  Right=" << rx_ControlFrame->RightSpeed << std::endl;
					navigator->driver->RemoteControllerWrite(rx_ControlFrame->LeftSpeed, rx_ControlFrame->RightSpeed);
					//navigator->driver->WriteArm(rx_ControlFrame->ArmVerticalSpeed, rx_ControlFrame->ArmHorizontalSpeed);

					//if (rx_ControlFrame->ArmVerticalSpeed != last_arm_vertical_speed || rx_ControlFrame->ArmHorizontalSpeed != last_arm_horizontal_speed)
					//{
					//	last_arm_vertical_speed = rx_ControlFrame->ArmVerticalSpeed;
					//	last_arm_horizontal_speed = rx_ControlFrame->ArmHorizontalSpeed;
					//	navigator->driver->WriteArm(rx_ControlFrame->ArmVerticalSpeed, rx_ControlFrame->ArmHorizontalSpeed);
					//}
				}
				else
				{
					navigator->driver->RemoteControllerActivate = false;
				}
			}
		}
	}
	catch (...)
	{
		std::cerr << "ERROR: CONTROLLER COMMUNICATION FAILS" << endl;
		exit(-9090);
	}
	return nullptr;
}
