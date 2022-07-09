#pragma once
#include <string>
#include "GPS.h"
#include <opencv2/core.hpp>
#include "boost/process/spawn.hpp"
#include "SharedGeoImage.h"

class Cropmapping_Process
{
public:
	std::string name;
	int camera_index = -1;
	std::string parameters;

	Cropmapping_Process(std::string name);
	int Start(int image_size);
	void Share(double latitude, double longitude, double heading, cv::Mat image);

private:
	SharedGeoImage shared;
	int image_size;
};

