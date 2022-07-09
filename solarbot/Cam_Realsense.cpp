#include "Cam_Realsense.h"

Cam_Realsense::Cam_Realsense(std::string name) :align_to_color(RS2_STREAM_COLOR) 
{
	this->name = name;
}

Cam_Realsense::Cam_Realsense(std::string name, std::string id_cam) : align_to_color(RS2_STREAM_COLOR)
{
	this->name = name;
	this->id_cam = id_cam;
}

Cam_Realsense::~Cam_Realsense() {}

int Cam_Realsense::Open(std::string id_cam, int width_bgr, int height_bgr, int width_depth, int height_depth, int fps)
{
	this->width_bgr = width_bgr;
	this->height_bgr = height_bgr;
	this->width_depth = width_depth;
	this->height_depth = height_depth;
	this->fps = fps;

	


	return Open(id_cam);
}

int Cam_Realsense::Open(std::string id_cam)
{	
	this->id_cam = id_cam;
	return Open();
}

int Cam_Realsense::Open()
{
	if (id_cam != "")
	{
		temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.2f);
		thresh_filter.set_option(RS2_OPTION_MIN_DISTANCE, 0.1);
		thresh_filter.set_option(RS2_OPTION_MAX_DISTANCE, 3.2);

		// Declare depth colorizer for pretty visualization of depth data
		cfg.enable_device(id_cam);
		cfg.enable_stream(RS2_STREAM_COLOR, width_bgr, height_bgr, RS2_FORMAT_BGR8, fps);
		cfg.enable_stream(RS2_STREAM_DEPTH, width_depth, height_depth, RS2_FORMAT_Z16, fps);
		//cfg.enable_stream(RS2_STREAM_ACCEL);
		//cfg.enable_stream(RS2_STREAM_GYRO);
		//if (pipe.start(cfg)) return err::ERR_OPENING_CAMERA;
		profile = pipe.start(cfg);
		rgb_sensor = profile.get_device().query_sensors()[1];
		std::cout << "Start CAM " << id_cam << " (" << name << ") -> OK" << std::endl;

		// filter settings
		color_map.set_option(RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED, 0);
		color_map.set_option(RS2_OPTION_MAX_DISTANCE, max_depth);
		color_map.set_option(RS2_OPTION_MIN_DISTANCE, min_depth);

		mat_bgr.create(height_bgr, width_bgr, CV_8UC3);
		mat_depth.create(height_depth, width_depth, CV_16UC1);

		//InUse = true;
		//rgb_sensor.set_option(rs2_option::RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, false);
		////rgb_sensor.set_option(rs2_option::RS2_OPTION_EXPOSURE, 6);
		//rgb_sensor.set_option(rs2_option::RS2_OPTION_WHITE_BALANCE, 4909);

		//rgb_sensor.set_option(rs2_option::RS2_OPTION_SATURATION, 70);
		rgb_sensor.set_option(rs2_option::RS2_OPTION_GAMMA, 500);
		//rgb_sensor.set_option(rs2_option::RS2_OPTION_ENABLE_AUTO_EXPOSURE, true);
	}
	else
	{
		std::cout << "Error Name Cam " << std::endl;
		return err::ERR_INVALID_PARAMETER;
	}
	return err::OK;
}

void Cam_Realsense::Close()
{
	pipe.stop();
}

bool Cam_Realsense::Read()
{
	return Read(&mat_bgr, &mat_depth);
}

bool Cam_Realsense::Read(cv::Mat* mat_bgr, cv::Mat* mat_depth, cv::Mat* mat_bgr_depth)
{
	if (!pipe.poll_for_frames(&fs))return false;
	
	fs = align_to_color.process(fs);	

	frame_bgr = fs.get_color_frame();
	frame_depth = fs.get_depth_frame();
	
	frame_depth = thresh_filter.process(frame_depth);
	frame_depth = spat_filter.process(frame_depth);
	frame_depth = temp_filter.process(frame_depth);
		
	if ( !frame_depth || !frame_bgr)return false;
	
	mat_bgr->data = (uchar*)frame_bgr.get_data();
	mat_depth->data = (uchar*)frame_depth.get_data();

	if (mat_bgr_depth != NULL)
	{
		rs2::frame frame_depth_bgr = frame_depth.apply_filter(color_map);
		mat_bgr_depth->data = (uchar*)frame_depth_bgr.get_data();
	}
	
	return true;
}



float Cam_Realsense::get_distance(cv::Point point)
{	
	return  fs.get_depth_frame().get_distance(point.x, point.y);;
}

rs2::frame Cam_Realsense::get_frame_bgr()
{
	return rs2::frame(frame_bgr);
}

rs2::frame Cam_Realsense::get_frame_depth()
{
	return rs2::frame(frame_depth);
}


