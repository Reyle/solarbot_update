#include "Detect_object.h"


Detect_object::Detect_object()
{
}

Detect_object::Detect_object(std::string camera_name, bool DEBUG_DETECT_OBJECT)
{
	this->camera_name = camera_name;
	this->DEBUG_DETECT_OBJECT = DEBUG_DETECT_OBJECT;

	if (DEBUG_DETECT_OBJECT)
	{
		cv::namedWindow(camera_name);
		cv::setMouseCallback(camera_name, DebugMouseCallbackFunction, NULL);
	}
}

Detect_object::Detect_object(std::string camera_name, std::vector<Detect_object::Zone> *zones, bool DEBUG_DETECT_OBJECT)
{
	this->camera_name = camera_name;
	this->zones = *zones;
	this->DEBUG_DETECT_OBJECT = DEBUG_DETECT_OBJECT;

	if (DEBUG_DETECT_OBJECT)
	{
		cv::namedWindow(camera_name);
		cv::setMouseCallback(camera_name, DebugMouseCallbackFunction, NULL);
	}
}

Detect_object::~Detect_object()
{
}


std::vector<uint16_t> Detect_object::search(cv::Mat* mat_depth, cv::Mat* mat_bgr)
{
	if (DEBUG_DETECT_OBJECT)
	{
		if (mat_bgr == NULL)
			mat_draw = cv::Mat::zeros(mat_depth->size().height, mat_depth->size().width, CV_8UC3);
		else
			mat_draw = (*mat_bgr).clone();
	}

	int contador_zonas = 0;
	cv::Scalar colores[] = { cv::Scalar(0, 0, 255), cv::Scalar(0, 255, 255), cv::Scalar(255, 0, 0), cv::Scalar(255, 0, 255), cv::Scalar(255, 255, 255) };
	

	//cv::Mat mat_depth = (*_mat_depth).clone();

	//int rows = mat_depth.rows;
	//int cols = mat_depth.cols;
	//float factor;
	//for (int c = 0; c < cols; c++)
	//{
	//	factor = ((float)c) / (151.4 + c);
	//	for (int r = 0; r < rows; r++)
	//	{
	//		mat_depth.at<uint16_t>(r, c) = (uint16_t)(mat_depth.at<uint16_t>(r, c) * factor);
	//	}
	//}
	
	std::vector<uint16_t> results;
	
	//Search for each zone in the zones vector
	for (Detect_object::Zone& zone : zones)
	{

		uint16_t result;
		cv::Mat roi = (*mat_depth)(zone.ROI); //Create the ROI		
		cv::Mat roi_inRange;
		cv::inRange(roi, zone.min_distance, zone.max_distance, roi_inRange); //Apply the InRage to get only the pixels with distance less than the warning distance
		//cv::imshow(std::to_string(zone.warning_distance), roi_inRange);

#ifdef DEBUG_DETECT_OBJECT
		//cv::imshow(cam->name + ":" + std::to_string(contador_zonas), roi_inRange);
		//cv::Mat convertedSrc(roi.rows, roi.cols, CV_8UC3, colores[contador_zonas]);
		//cv::cvtColor(roi_inRange, convertedSrc, cv::COLOR_GRAY2BGR);
		//cv::Mat roid_ = mat_draw(zone.ROI);
		//convertedSrc.copyTo(roid_);
#endif

		std::vector<std::vector<cv::Point>> every_countours;
		std::vector<std::vector<cv::Point>> filtered_countours;
		std::vector<cv::Vec4i> every_hierarchy;
		std::vector<cv::Vec4i> filtered_hierarchy;

		//Find all contours
		cv::findContours(roi_inRange, every_countours, every_hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);


		//Filtered the countours  
		for (int i = 0; i < every_countours.size(); i++)
		{
			double area = cv::contourArea(every_countours[i]);
			if (area > zone.min_area)
			{
				filtered_countours.push_back(every_countours[i]);
				filtered_hierarchy.push_back(every_hierarchy[i]);
			}
		}

		//std::cout << "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK  " << filtered_countours.size() << std::endl;

		//********************  find the closest poit with it value for each countour ****************************
		int cols = roi.cols;
		int rows = roi.rows;
		int16_t v = 0;
		std::vector<cv::Point> closest_points;
		std::vector<int16_t> closest_points_value;
		for (std::vector<cv::Point>& countour : filtered_countours)
		{
			closest_points_value.push_back(10000);
			closest_points.push_back(cv::Point(0, 0));
		}


		int filtered_countours_count = filtered_countours.size();
		for (int r = 0; r < rows; r++) //for each rows
		{
			for (int c = 0; c < cols; c++) //for each colums
			{
				if (roi_inRange.at<unsigned char>(r, c) == 255) //if pixel is active in the roi_inRange mat
				{
					for (int i = 0; i < filtered_countours_count; i++) // for each filtered contour
					{
						if (cv::pointPolygonTest(filtered_countours[i], cv::Point2f(c, r), false) >= 0) //if point inside cotour
						{
							v = roi.at<int16_t>(r, c); //get the distance value of the pixel
							if (v < closest_points_value[i]) //update the closest point
							{
								closest_points_value[i] = v;
								closest_points[i] = cv::Point(c, r);
							}
							break; //beacause the point can only be inside one contour
						}
					}
				}
			}
		}
		//********************  END find the closest poit with it value for each countour ****************************



		//Update the result depending if the distance is OK, warning or critical
		result = UINT16_MAX;
		for (int i = 0; i < filtered_countours_count; i++)
		{
			if (closest_points_value[i] >= zone.min_distance && closest_points_value[i] <= zone.max_distance)
			{
				if(closest_points_value[i] < result)
					result = closest_points_value[i];
			}
		}

		//Add the result to the results
		results.push_back(result);

		//Just for visualization!!!
		if (DEBUG_DETECT_OBJECT)
		{
			for (int i = 0; i < filtered_countours_count; i++)
			{
				closest_points[i].x += zone.ROI.x;
				closest_points[i].y += zone.ROI.y;

				for (int g = 0; g < filtered_countours[i].size(); g++)
				{
					filtered_countours[i][g].x += zone.ROI.x;
					filtered_countours[i][g].y += zone.ROI.y;
				}

				cv::drawContours(mat_draw, filtered_countours, i, colores[contador_zonas], 2, 8, filtered_hierarchy, 0, cv::Point(0, 0));
				cv::circle(mat_draw, closest_points[i], 4, colores[contador_zonas], -1, 8, 0);

			}

			cv::rectangle(mat_draw, zone.ROI, colores[contador_zonas], 1, 8);

			contador_zonas++;
			if (contador_zonas > 4)
				contador_zonas = 4;
		}
	}


	if (DEBUG_DETECT_OBJECT)
	{
		if (&camera_name != NULL && camera_name.length() > 0)
		{
			std::string windows_name = camera_name;
			//std::cout << "CAM= " << windows_name << std::endl;
			cv::imshow(windows_name, mat_draw);
			cv::waitKey(1);
		}

	}
	return results;
	
}

//#ifdef DEBUG_DETECT_OBJECT	
void Detect_object::DebugMouseCallbackFunction(int event, int x, int y, int flags, void* userdata)
{
	if (event == cv::EVENT_LBUTTONDOWN)
	{
		//cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;

		std::cout << "PIXEL " << x << ", " << y <<  std::endl;
	}
	else if (event == cv::EVENT_RBUTTONDOWN)
	{
		//cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
	}
	else if (event == cv::EVENT_MBUTTONDOWN)
	{
		//cout << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
	}
	else if (event == cv::EVENT_MOUSEMOVE)
	{
		//std::cout << "PIXEL " << x << ", " << y  << std::endl;
	}
}
//#endif // DEBUG_DETECT_OBJECT	