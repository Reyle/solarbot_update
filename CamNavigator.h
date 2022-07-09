#pragma once
#include <string>
#include <opencv2/core.hpp> // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui.hpp> // Video write
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

using namespace std;
using namespace cv;

class CamNavigator
{
public:

	struct Result {
		bool readOK;
		double angle;
		int quaity;
	};

	Mat dsp;

	CamNavigator();
	~CamNavigator();
	bool Open(string device);
	CamNavigator::Result ReadImage();

private:
	double img_zoom;
	int img_width;
	int img_height;

	int search_width;
	int search_height;
	int search_left;
	int search_top;

	int interest_width;
	int interest_height;
	int interest_left;
	int interest_top;

	double resize_factor_x;
	double resize_factor_y;
	Rect search_rect;
	Rect interest_rect;

	// Split ROI into 16 subregions of 420x16 pixels
	// Vector of Matrices for the threshold segments
	vector <Mat> strips;

	// Vector of Vectors of Points for areas centers
	vector<vector<Point2f>> centers;

	//Masking points for filled triangle
	Point tr_left[1][3];
	
	const Point* ppt_tr_left[1];
	int npt_tr_left[1];
	Point tr_right[1][3];
	const Point* ppt_tr_right[1];
	int npt_tr_right[1];

	Mat croppedWA;
	Mat croppedTP;

	// Convert frame from BGR to HSV colorspace
	Mat HSV;
	Mat HSVTP;

	// Filter HSV image between values and store filtered image to threshold matrix
	Mat threshld;
	Mat threshldTP;

	int H_MIN = 21;
	int H_MAX = 117;
	int S_MIN = 51;
	int S_MAX = 256;
	int V_MIN = 72;
	int V_MAX = 256;

	cv::VideoCapture capture;
	Mat erodeElement;
	Mat dilateElement;

	void morphOps(Mat& thresh);
	vector<Point2f> trackFilteredObject(Mat threshold, Mat& cameraFeed, Point offs);
	void separator(vector<vector<Point2f>>& centers, vector<vector<Point2f>>& lineas);
	void linearizer(Point2f& A, Point2f& B, Point2f& A0, Point2f& B0);

};

