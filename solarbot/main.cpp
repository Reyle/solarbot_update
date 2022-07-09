#include <cstdio>
#include <iostream>
#include <string>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>

#include "MasterApp.h"
#include "Solarbot.h"
#include "CamNavigator.h"

//#include "RobotNetServer.h" //BORRAR
//#include "Head.h" //BORRAR

//ATECION NO BORRAR, EJECUTAR ESTO PARA GARANTIZAR QUE SE RECONECTE LA WIFI!!!!!!!!!!!!!!!!
//1- Go to /etc/ifplugd/action.d/ and rename the ifupdown file to ifupdown.original
//2- Then do: cp /etc/wpa_supplicant/ifupdown.sh ./ifupdown
//3- Finally reboot


using namespace std;


volatile int ctrl;
void signal_handler(int signo)
{
	ctrl = 0;
}

char getch() {
	char buf = 0;
	struct termios old = { 0 };
	if (tcgetattr(0, &old) < 0)
		perror("tcsetattr()");
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &old) < 0)
		perror("tcsetattr ICANON");
	if (read(0, &buf, 1) < 0)
		perror("read()");
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	if (tcsetattr(0, TCSADRAIN, &old) < 0)
		perror("tcsetattr ~ICANON");
	return (buf);
}

void print_usage()
{
	cout << "\nUsage:" << endl;
	cout << "\t-m\tNavigation mode: waypoints, visual, line" << endl;
	cout << "\t-w\tWaypoints file" << endl;
	cout << "\t-c\tNavigation camera (default: /dev/video0)" << endl;
	cout << "\t-n\tCamera navigation angle factor" << endl;
	cout << "\t-R\tRows to analize" << endl;
	cout << "\t-r\tCrop row width (in pixels)" << endl;
	cout << "\t-z\tCamera zoom" << endl;
	cout << "\t-p\tPerspective Diff (in pixels)" << endl;
	cout << "\t-h\tHorizontal offset (in pixels)" << endl;
	cout << "\t-s\tShow camera" << endl;
	cout << "\t-v\tFile name to save the video in ~/videos" << endl;
	cout << "\t-f\tStart execution without ask" << endl;
	cout << "\t-t\tTransmit navigation video (yes or no)" << endl;

}

int main(int argc, char* argv[])
{
	//Navigator nav;
	//nav.LoadWaypoints("/home/solarbot/waypoints/terra.txt");

	//Navigator::Point p;
	//for (int i = 0; i < nav.waypoints_size; i++)
	//{
	//	p = nav.GetDestinationPoint(nav.waypoints[i].latitude, nav.waypoints[i].longitude, 1.0, 302);
	//	printf("P%0.7f,%0.7f,1000\n", p.lat, p.lon);
	//}
	//
	//return 0;

	//int cc = sizeof(Telemetry::Frame);
	//Head head;
	//int r = head.Open("/dev/ttyHead", 57600);
	//if (r != 0)
	//{
	//	std::cout << "ERROR opening head sensors..." << r << std::endl;
	//	return r;
	//}

	//int i = 0;
	//r = head.ReadSensors();
	//while (r != 0 && i < 10)
	//{
	//	r = head.ReadSensors();
	//	usleep(10000);
	//	i++;
	//}

	//if (i >= 10)
	//{
	//	std::cout << "ERROR reading head sensors..." << std::endl;
	//	return -1;
	//}
	//else
	//{
	//	std::cout << "Reading head sensors start errors count=" << i << std::endl;
	//}

	//head.SetEyes(Head::E_ANGRY, false, false, false);

	////while (r == 0)
	////{
	////	r = head.ReadSensors();
	////	printf("X=%d\tY=%d\tZ=%d\r\n", head.data.mx, head.data.my, head.data.mz);
	////	usleep(50000);
	////}
	////return 0;

	//if (r == 0)
	//{
	//	//Para Aracatuba la declinacion magnetica es 20.08W
	//	//head.SetMagneticHeadingCalibration(-27, 20, -20.17 - 5);
	//	//Xoffset=110     Yoffset=-46     Zoffset=332     Xscale=0.66     Yscale=0.70     Zscale=18.24
	//	//Xoffset=107     Yoffset=-46     Zoffset=333     Xscale=0.66     Yscale=0.70     Zscale=17.96

	//	//Xoffset=8.74    Yoffset=-95.68  Zoffset=27.14   Xscale=0.90     Yscale=0.92     Zscale=1.24

	//	//head.SetMagneticHeadingCalibration(110, -46, -55, 1.05, 1.06, 1.01, -20.17 - 3.0);
	//	//head.SetMagneticDeclination(-20.17 + 3.0);
	//	head.SetMagneticDeclination(-20.17 -3);
	//	//head.SetMagneticHeadingCalibration(8.74f, -95.68f, 27.14f, 0.90, 0.92, 1.24, 0);
	//	while (1)
	//	{
	//		if (!head.ReadSensors())
	//		{
	//			printf("H=%0.1f | P=%0.1f | R=%0.1f\n", head.data.Heading, head.data.Pitch, head.data.Roll);
	//			//printf("%0.1f\n", head.getMagneticHeading());
	//			usleep(250000);
	//		}
	//	}
	//}
	//return 0;

	ctrl = 1;
	signal(SIGINT, signal_handler);

    printf("Solarbot 0.4.0 2021-11-05\n");
	std::cout << "sizeof(double)=" << sizeof(double) << " sizeof(float)=" << sizeof(float) << " sizeof(int)=" << sizeof(int) << std::endl;

	////RobotNetServer rs;

	////rs.start(4000);
	////
	////while (ctrl)
	////	usleep(10000);

	////return 0;


	////CamNavigator navigator;

	////if (navigator.Open("/home/solarbot/videos/v5.avi"))
	//////if (navigator.Open("/dev/video0"))
	////{
	////	CamNavigator::Result r;
	////	for (;;)
	////	{
	////		r = navigator.ReadImage();

	////		cout << "r=" << r.used_row << "\ta=" << r.angle << "\tq=" << r.quaity << endl;

	////		imshow("dsp", navigator.dsp);
	////		if (waitKey(1) == 27)
	////		{
	////			navigator.~CamNavigator();
	////			break;
	////		}
	////	}
	////}

	string mode = "standby";
	string waypoints_filename = "";
	//tring camera_path = "/dev/video0";
	int navigation_angle_factor = 25;
	int rows = 5;
	int row_width = 124;
	double img_zoom = 1.3;
	int perspective_diff = 120;
	int horizontal_offset = 40;
	string save_filename;
	bool transmit_navigation_video = false;


	int opt;
	int a_m = 0;
	int a_w = 0;
	//int a_c = 0;
	int a_n = 0;
	int a_R = 0;
	int a_r = 0;
	int a_z = 0;
	int a_p = 0;
	int a_h = 0;
	int a_s = 0;
	int a_v = 0;
	int a_f = 0;
	int a_t = 0;
	int a_o = 0;
	while ((opt = getopt(argc, argv, "m:w:c:n:R:r:z:p:h:sv:t:of")) != EOF)
		switch (opt)
		{
		case 'm': a_m = 1; mode = string(optarg); break;
		case 'w': a_w = 1; waypoints_filename = string(optarg); break;
		//case 'c': a_c = 1; camera_path = string(optarg); break;
		case 'n': a_n = 1; navigation_angle_factor = atoi(optarg); break;
		case 'R': a_R = 1; rows = atoi(optarg); break;
		case 'r': a_r = 1; row_width = atoi(optarg); break;
		case 'z': a_z = 1; img_zoom = atof(optarg); break;
		case 'p': a_p = 1; perspective_diff = atoi(optarg); break;
		case 'h': a_h = 1; horizontal_offset = atoi(optarg); break;
		case 's': a_s = 1; break;
		case 'v': a_v = 1; save_filename = string(optarg); break;
		case 'o': a_o = 1; break;
		case 'f': a_f = 1; break;
		//case 't': a_t = 1; if (optarg != NULL && string(optarg) == "no") transmit_navigation_video = false; else transmit_navigation_video = true;  break;
		case 't': a_t = 1; transmit_navigation_video = string(optarg) == "yes"; break;
		case '?': print_usage(); abort(); break;
		default: cout << endl; abort();
		}

	if (mode != "standby" && mode != "waypoints" && mode != "visual" && mode != "line")
	{
		cout << "ERROR: Invalid mode. Valid modes are waypoints, visual or line" << endl;
		abort();
	}


	cout << "Current configuration:" << endl;
	cout << "\t(m)Navigation mode = " << mode << endl;
	cout << "\t(w)Waypoints file = " << (a_w ? ("/home/solarbot/waypoints/" + waypoints_filename) : "NONE") << endl;
	//cout << "\t(c)Navigation camera = " << camera_path << endl;
	cout << "\t(n)Camera navigation angle factor = " << navigation_angle_factor << endl;
	cout << "\t(R)Rows to analize = " << rows << endl;
	cout << "\t(r)Crop row width = " << row_width << endl;
	cout << "\t(z)Camera zoom = " << img_zoom << endl;
	cout << "\t(p)Perpective Diff = " << perspective_diff << endl;
	cout << "\t(h)Horizontal offset = " << horizontal_offset << endl;
	cout << "\t(s)Show video = " << (a_s ? "YES" : "NO") << endl;
	cout << "\t(o)Debug object detection = " << (a_o ? "YES" : "NO") << endl;
	cout << "\t(v)Save video = " << (a_v ? ("/home/solarbot/videos/" + save_filename) : "NO") << endl;
	if(a_t == 1)
		cout << "\t(s)Transmit navigation video = " << (transmit_navigation_video ? "YES" : "NO") << endl;

	
	
	if (a_f == 0)
	{
		cout << "Excecute? yes (Y) or no (N) ?" << endl;
		char key;
		key = getch();
		if (key != 'y' && key != 'Y')
		{
			abort();
		}
	}
	
	cout << "Solarbot Started!" << endl;

	Solarbot solarbot;

	if (a_o == 1)//It is here because need to be before to call ReadConf()
		solarbot.debug_detect_object = true;

	solarbot.ReadConf();//First read config so we can override anyone if necessary

	if (a_w == 1) //if waypoints file is passed
	{
		if (a_m == 1 && mode != "waypoints")
		{
			cout << "ERROR: Invalid mode. If the waypoints file is passed then the passed mode shoud be 'waypoints'" << endl;
			abort();
		}
		else
		{
			//Set waypoints mode
			mode = "waypoints";			
			solarbot.setWayPointsFile("/home/solarbot/waypoints/" + waypoints_filename);
		}
	}

	solarbot.setNavigationMode(mode);
	solarbot.setShowVideo(a_s);
	solarbot.setSaveVideo(a_v, save_filename);

	if(a_n == 1)
		solarbot.setCamNavigationAngleFactor(navigation_angle_factor);

	if (a_R == 1)
		solarbot.navigator->cam.rows = rows;

	if(a_r == 1)
		solarbot.navigator->cam.row_width = row_width;

	if (a_z == 1)
		solarbot.navigator->cam.img_zoom = img_zoom;

	if (a_p == 1)
		solarbot.navigator->cam.perspective_diff = perspective_diff;

	if(a_h == 1)
		solarbot.navigator->cam.horizontal_offset = horizontal_offset;

	if(a_t == 1)
		solarbot.TransmitNavigationVideo = transmit_navigation_video;

	

	MasterApp master(2020, &solarbot, &ctrl);
	
	int result = solarbot.run(&ctrl);

	master.Close();

	return result;

	//string str;
	//getline(cin, str);
    //return 0;
}