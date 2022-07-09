#include "Cropmapping_Process.h"
#include <iostream>

Cropmapping_Process::Cropmapping_Process(std::string name)
{
	this->name = name;
	this->image_size = image_size;
}

int Cropmapping_Process::Start(int image_size)
{
	this->image_size = image_size;
	int r = shared.Open("camera_" + name, image_size, true);
	//if (r == 0)
	//	boost::process::spawn("/home/solarbot/solarbot/cropmapping/bin/ARM64/Debug/cropmapping.out", (name + ", " + parameters).c_str());
	//else
	//	std::cerr << "ERROR: Opening SharedGeoImage" << std::endl;
	
	return r;
}

void Cropmapping_Process::Share(double latitude, double longitude, double heading, cv::Mat image)
{
	int r = shared.Write(latitude, longitude, heading, image.data);

	if (r != 0)
		std::cout << "share ERROR" << std::endl;
}
