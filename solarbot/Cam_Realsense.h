#pragma once
//=====INCLUDE REAL SENSE=====
#include<librealsense2/rs.hpp>

//=====INCLUDES OPENCV========
#include<opencv2/opencv.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>

//======OTHER INCLUDES========
#include <thread>
#include <iostream>
#include "err.h"

//Utilitty class to open, read and close a Realsense Camera

class Cam_Realsense
{
public:
	Cam_Realsense(std::string name);
	Cam_Realsense(std::string name, std::string id_cam);
	~Cam_Realsense();

	std::string name;
	std::string id_cam;

	int Open(std::string id_cam, int width_bgr, int height_bgr, int width_depth, int height_depth, int fps);
	int Open(std::string id_cam);
	int Open();
	void Close();

	bool Read();
	bool Read(cv::Mat* mat_bgr, cv::Mat* mat_depth, cv::Mat* mat_bgr_depth = NULL);
	

	float get_distance(cv::Point point);	
	rs2::frame get_frame_bgr();
	rs2::frame get_frame_depth();

	int width_bgr = 424;
	int height_bgr = 240;

	bool InUse = false;
	cv::Mat mat_bgr;
	cv::Mat mat_depth;

	rs2::sensor rgb_sensor;
	
private:

	rs2::frameset fs;
	rs2::pipeline pipe;
	rs2::pipeline_profile profile;
	rs2::align align_to_color;
	rs2::config cfg;
	rs2::frame frame_bgr;
	rs2::frame frame_depth;

	rs2::spatial_filter spat_filter;	
	rs2::temporal_filter temp_filter;
	rs2::threshold_filter thresh_filter;
	rs2::colorizer color_map;

	float min_depth = 0.29f;
	float max_depth = 5.0f;

	int width_depth = 480;
	int height_depth = 270;

	int fps = 15;


};

