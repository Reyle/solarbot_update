#pragma once

//#define DEBUG_CAM_NAVIGATOR

#include <string>
#include <opencv2/core.hpp> // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui.hpp> // Video write
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include "UdpVideoSender.h"

using namespace std;
using namespace cv;

class CamNavigator
{
public:

	bool UseRealSenseCamera = true;

	//Struct to return result
	struct Result {
		bool readOK;
		double angle;
		int quality;
		int used_row;
	};


	Mat dsp; // Mat of the image after zoomed
	Mat debug_mat;// Debug mat to paint the result
	

	float navigation_angle_factor = 25.0;
	int row_width = 124;
	int rows = 6;
	double img_zoom = 1.3;
	int perspective_diff = 120;
	int horizontal_offset = 40;
	int vertical_offset = 0;

	int H_MIN = 21;
	int H_MAX = 117;
	int S_MIN = 51;
	int S_MAX = 256;
	int V_MIN = 72;
	int V_MAX = 256;

	vector<vector<Point2f>> row_lines;	

	bool DEBUG_CAM_NAVIGATOR = true;
	bool SHOW_CAM_NAVIGATOR = false;
	bool SAVE_CAM_NAVIGATOR = false;
	std::string save_video_filename;

	CamNavigator();
	~CamNavigator();
	bool Init(int capture_width, int capture_height);
	CamNavigator::Result ProcessImage(Mat dsp);
	void AddDebugInformationToImage(std::string msg);

	cv::VideoWriter video_writer;

	struct sockaddr_in debug_udp_tx_address;
	int debug_udp_tx_socketfd;

	UdpVideoSender* udp_video_sender; 
	void SendDebugImg2UDPServer();

private:

	//int capture_width = 420;
	//int capture_height = 320;
	int capture_width = 424;
	int capture_height = 240;

	int img_width; //width of the result image after zoomed
	int img_height; //Height of the result image after zoomed

	bool has_center_row = true;

	const int search_width = 420;
	const int search_height = 256;
	int search_left; //left of the search region referenced to the image
	int search_top; //top of the search regio referenced to the image

	int interest_width;
	int interest_height;
	int interest_left;
	int interest_top;

	Rect search_rect;// Rectangle of the Search Region (the big rectangle using for the real work)
	Rect interest_rect;// Rectangle of the Interest Region (the litle rectangle, using for color calibration)

	Mat search_region;//Mat of the Search Region	
	Mat HSV; // Mat to convert frame from BGR to HSV colorspace
	Mat threshld; //Threshold matrix after filter HSV image between values the values to select the crop 
	

	//Perspective transform matrixes
	Mat PerspectiveTransformMatrix;
	Mat InvPerspectiveTransformMatrix; //just for debug

	// Vector of Matrices to split Search Region into 16 subregions of 420x16 pixels
	vector <Mat> strips;

	// Vector of Vectors of Points for areas centers
	vector<vector<Point2f>> centers;



	//Masking points for filled triangle
	//Point tr_left[1][3];

	//const Point* ppt_tr_left[1];
	//int npt_tr_left[1];
	//Point tr_right[1][3];
	//const Point* ppt_tr_right[1];
	//int npt_tr_right[1];


	

	

	vector<vector<Point2f>> separation_lines;

	cv::VideoCapture capture;
	Mat erodeElement;
	Mat dilateElement;

	void morphOps(Mat& thresh);
	vector<Point2f> findCenters(Mat threshold, Point offs);
	void separator(vector<vector<Point2f>>& centers, vector<vector<Point2f>>& separatedCenters);
	void getRowLines(vector<vector<Point2f>>& separatedCenters);
	bool fexists(const std::string& filename);

	rs2::pipeline pipe;
	rs2::config cfg;
	rs2::align* align_to_color;


};

