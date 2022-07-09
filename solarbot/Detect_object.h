#pragma once

//=====INCLUDES OPENCV========
#include<opencv2/opencv.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>

//======OTROS INCLUDES========
//#include <thread>
#include <iostream>

//#define DEBUG_DETECT_OBJECT


class Detect_object
{
public:

	bool DEBUG_DETECT_OBJECT = false;

	//The struct Zone is a rectangle with a warning and critical distance
	struct Zone
	{
		cv::Rect ROI;
		uint16_t min_distance;
		uint16_t max_distance;
		int min_area;
	};


	std::string camera_name;
	int camera_id = -1;
	std::vector<Detect_object::Zone> zones;
	
	//The constructor need an already oppened camera (cam parameter) and a vector of zones
	Detect_object();
	Detect_object(std::string camera_name, bool DEBUG_DETECT_OBJECT);
	Detect_object(std::string camera_name, std::vector<Detect_object::Zone> *zones, bool DEBUG_DETECT_OBJECT);
	~Detect_object();

	
	//Search for each zone and return a vector of the same size of the member zones with the zones
	//result in ok, warning or critical
	std::vector<uint16_t> search(cv::Mat *mat_depth, cv::Mat* _mat_bgr = NULL);


private:


//#ifdef DEBUG_DETECT_OBJECT	
//	static void DebugMouseCallbackFunction(int event, int x, int y, int flags, void* userdata);
//#endif

	cv::Mat mat_draw;

	static void DebugMouseCallbackFunction(int event, int x, int y, int flags, void* userdata);


};

