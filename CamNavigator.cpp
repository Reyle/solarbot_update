#include <iostream> // for standard I/O

#include "CamNavigator.h"
#include "unistd.h"
#include <opencv2/core/utils/logger.hpp>

using namespace std;
using namespace cv;

CamNavigator::CamNavigator()
{
	vector <Mat> temp_strips(16);
	strips = temp_strips;

	vector<vector<Point2f>> temp_centers(16);
	centers = temp_centers;

	tr_left[0][0] = Point(0, 0);
	tr_left[0][1] = Point(0, 192);
	tr_left[0][2] = Point(105, 0);

	ppt_tr_left[0] = tr_left[0];
	npt_tr_left[0] = 3;

	tr_right[0][0] = Point(420, 0);
	tr_right[0][1] = Point(420, 192);
	tr_right[0][2] = Point(315, 0);

	ppt_tr_right[0] = tr_right[0];
	npt_tr_right[0] = 3;

	//the element chosen here is a 11px by 22px rectangle
	erodeElement = getStructuringElement(MORPH_RECT, Size(11, 22)); //Size(11, 11));

	//dilate with larger element so make sure object is nicely visible
	dilateElement = getStructuringElement(MORPH_RECT, Size(7, 14)); //Size(7, 7));
}

CamNavigator::~CamNavigator()
{
	capture.release();
	capture.~VideoCapture();
}


bool CamNavigator::Open(string device)
{
	utils::logging::setLogLevel(utils::logging::LogLevel::LOG_LEVEL_INFO);
	capture.setExceptionMode(true);

	capture.open(device);
	//capture.open(1);
	//capture = VideoCapture(0);
	if (!capture.isOpened())
	{
		return false;
	}
	else
	{
		img_zoom = 1.1;
		img_width = img_zoom * 420;
		img_height = img_zoom * 320;
		search_width = 420;
		search_height = 256;
		search_left = (img_width / 2) - (search_width / 2);
		search_top = img_height - search_height;

		interest_width = 100;
		interest_height = 50;
		interest_left = (img_width / 2) - (interest_width / 2);
		interest_top = img_height - interest_height;

		if (search_left < 0) search_left = 0; //por si acaso
		if (search_top < 0)	search_top = 0; // por si acaso
		if (interest_left < 0) interest_left = 0; //por si acaso
		if (interest_top < 0)	interest_top = 0; // por si acaso


		resize_factor_x = img_width / capture.get(CAP_PROP_FRAME_WIDTH);
		resize_factor_y = img_height / capture.get(CAP_PROP_FRAME_HEIGHT);
		search_rect = Rect(search_left, search_top, search_width, search_height);
		//Rect search_rect = Rect(0, 59, 420, 256);
		//Rect interest_rect = Rect(160, 206, 100, 50);
		interest_rect = Rect(interest_left, interest_top, interest_width, interest_height);

		return true;
	}
}

CamNavigator::Result CamNavigator::ReadImage()
{
	CamNavigator::Result result;
	
	result.readOK = capture.read(dsp);

	if (result.readOK)
	{
		resize(dsp, dsp, Size(), resize_factor_x, resize_factor_y, cv::INTER_LANCZOS4);

		// Select region of interest, separate Wide-angle from Telephoto image
		croppedWA = dsp(search_rect);
		croppedTP = dsp.clone()(interest_rect);

		// Convert frame from BGR to HSV colorspace
		cvtColor(croppedWA, HSV, COLOR_BGR2HSV);
		cvtColor(croppedTP, HSVTP, COLOR_BGR2HSV);

		// Filter HSV image between values and store filtered image to threshold matrix
		inRange(HSV, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), threshld);
		inRange(HSVTP, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), threshldTP);

		// Draw solid triangles and lines to fit ROI
		fillPoly(threshld, ppt_tr_left, npt_tr_left, 1, Scalar(0, 0, 0), 8, 0);
		fillPoly(threshld, ppt_tr_right, npt_tr_right, 1, Scalar(0, 0, 0), 8, 0);
		line(threshld, Point(150, 0), Point(70, 256), Scalar(0, 0, 0), 4);
		line(threshld, Point(200, 0), Point(155, 256), Scalar(0, 0, 0), 4);
		line(threshld, Point(240, 0), Point(285, 256), Scalar(0, 0, 0), 4);
		line(threshld, Point(290, 0), Point(370, 256), Scalar(0, 0, 0), 4);

		// Go to "dilate" and "erode" image function.
		morphOps(threshld);

		// Split ROI into 16 subregions of 420x16 pixels
		for (int i = 0; i < strips.size(); i++) {
			strips[i] = threshld(Rect(0, 16 * i, 420, 16));
			// Find lane centers into the ROI's 16 subregions of 420x16 pixels
			//centers[i] = trackFilteredObject(strips[i], dsp, Point(110, 360 + 16 * i));
			centers[i] = trackFilteredObject(strips[i], dsp, Point(search_left, search_top + (16 * i)));
		}

		// Separate centers in left, center and right lanes
		// lineas[0]=left 1; lineas[1]=left 2; lineas[2]=center; lineas[3]=right 1;	lineas[4] = right 2
		vector<vector<Point2f>> lineas(5);
		separator(centers, lineas);

		// Find the line that fits each lane's detected points
		// Vec4f = (vx, vy, x0, y0); (vx, vy) is a normalized vector collinear to the line and (x0, y0) is a point on the line
		// linerizado[0]=left 1; linerizado[1]=left 2; linerizado[2]=center; linerizado[3] = right 1; linerizado[4] = right 2
		vector<Vec4f> linerizado(5);
		Point2f A0;
		Point2f B0;
		Point2f auxcenterWA;
		//Point2f auxcenterWB;
		for (int i = 0; i < linerizado.size(); i++) {
			if (lineas[i].size() >= 2) {
				fitLine(lineas[i], linerizado[i], DIST_L2, 0, 1, 0.01);

				Point2f p1 = Point2f(linerizado[i][2] - 60 * linerizado[i][0], linerizado[i][3] - 60 * linerizado[i][1]);
				Point2f p2 = Point2f(linerizado[i][2] + 60 * linerizado[i][0], linerizado[i][3] + 60 * linerizado[i][1]);
				linearizer(p1, p2, A0, B0);

				if (i == 2)
				{
					line(dsp, A0, B0, Scalar(0, 255, 255), 2, LINE_AA);
					auxcenterWA = B0; // center lane coordinate
					//auxcenterWB = B0; // center lane coordinate
					//float m = B0.x - A0.x;
					//cout << "m=" << std::fixed << std::setprecision(2) << m << endl;
					//std::stringstream stream;
					//stream << std::fixed << std::setprecision(2) << m;
					//std::string s = stream.str();
					//putText(dsp, s, Point((img_width / 2) - 40, search_top - 20), FONT_HERSHEY_PLAIN, 2, Scalar(255,255,255));
				}
				else
				{
					line(dsp, A0, B0, Scalar(255, 0, 0), 2, LINE_AA);
				}
			}
		}
		//line(dsp, Point((auxcenterWA.x - 302) * 12 + 640, 105), Point((auxcenterWA.x - 302) * 12 + 640, 615), Scalar(255, 0, 0), 2, LINE_AA); // Show in TP
		int auxcenter_x = (auxcenterWA.x - (img_width / 2)) * 3 + (img_width / 2);
		//line(dsp, Point(auxcenter_x, search_top), Point(auxcenter_x, img_height), Scalar(0, 255, 0), 4, LINE_AA); // Show in TP
		rectangle(dsp, Rect(auxcenter_x - 10, search_top - 25, 20, 10), Scalar(0, 255, 0), 8, LINE_AA); // Show in TP


			// Central row
			//line(dsp, Point(332, 105), Point(332, 615), Scalar(0, 255, 255), 1, CV_AA); // WA
		//line(dsp, Point(1002, 105), Point(1002, 615), Scalar(0, 255, 255), 2,	CV_AA); // TP
		// Location of TP inside WA
		//rectangle(dsp, Point(302, 360), Point(355, 400), Scalar(255, 255, 255), 1, CV_AA);
		//circle(dsp, Point(327, 380), 1, Scalar(255, 255, 255), 1, CV_AA);
		//circle(dsp, Point(960, 348), 4, Scalar(255, 255, 255), 4, CV_AA);

		rectangle(dsp, search_rect, Scalar(255, 0, 0), 1, 8, 0);
		rectangle(dsp, interest_rect, Scalar(255, 0, 0), 1, 8, 0);
		//imshow("dsp", dsp);
		//imshow("croppedTP", croppedTP);
		//imshow("HSV", HSV);
		//imshow("HSVTP", HSVTP);
	}

	return result;
}

void CamNavigator::morphOps(Mat& thresh) 
{
	//Opening Morphology Transformation: It is obtained by the erosion of an image	followed by a dilation.
	//create structuring element that will be used to "dilate" and "erode" image.
	// in this particular case, better results were obtained dilating first and then eroding	
	dilate(thresh, thresh, dilateElement);
	erode(thresh, thresh, erodeElement);
}

vector<Point2f> CamNavigator::trackFilteredObject(Mat threshold, Mat& cameraFeed, Point offs)
{
	Mat temp;
	threshold.copyTo(temp);
	//these two vectors are needed for the output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	//find contours of filtered image using openCV findContours function
	findContours(temp, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

	// Draw contours
	for (int i = 0; i < contours.size(); i++)
	{
		drawContours(cameraFeed, contours, i, Scalar(0, 0, 255), 2, 8, hierarchy, 0, offs);
	}

	// Get the moments
	vector<Moments> mu(contours.size());
	for (int i = 0; i < contours.size(); i++)
	{
		mu[i] = moments(contours[i], false);
	}

	// Get the mass centers:
	vector<Point2f> mc(contours.size());
	vector<Point2f> myvec;
	for (int i = 0; i < contours.size(); i++)
	{ // Offset to Point(160, 480)
		mc[i] = Point2f((mu[i].m10 / mu[i].m00) + offs.x, (mu[i].m01 / mu[i].m00) + offs.y);
		// Check that the point is inside the ROI
		if ((mc[i].x > offs.x) && (mc[i].x < offs.x + 420) && (mc[i].y > offs.y) && (mc[i].y < offs.y + 256)) {
			myvec.push_back(mc[i]);
		}
		else if ((mc[i].x > offs.x) && (mc[i].x < offs.x + 640) && (mc[i].y > offs.y) && (mc[i].y < offs.y + 439)) {
			myvec.push_back(mc[i]);
		}
	}

	for (int i = 0; i < myvec.size(); i++)
	{
		circle(cameraFeed, myvec[i], 4, Scalar(255, 0, 0), -1, 8, 0);
	}
	return myvec;
}

// Separate centers in left, center and right lanes
void CamNavigator::separator(vector<vector<Point2f>>& centers, vector<vector<Point2f>>& lineas)
{
	for (int i = 0; i < centers.size(); i++) {
		for (int j = 0; j < centers[i].size(); j++) {
			// Evaluate if a point P(x,y) is on the left side or right side of the line AB; A(x1, y1) B(x2, y2)
			// d = (x-x1)*(y2-y1)-(y-y1)*(x2-x1)
			// if d<0 ->left side; if d>0 ->right side; if d=0 ->on the line
			// Keep the x=110 y=360 (shift of ROI) addition terms for future reference.The commmented equations are equivalent to the uncommented ones
			// i.e. The uncommmented equations are the simplifications of the commented ones
			//double L1 = (centers[i][j].x - (150 + 110))*((256 + 360) - (0 + 360)) - (centers[i][j].y - (0 + 360)) * ((70 + 110) - (150 + 110));
			//double L2 = (centers[i][j].x - (200 + 110))*((256 + 360) - (0 + 360)) - (centers[i][j].y - (0 + 360)) * ((155 + 110) - (200 + 110));
			//double R1 = (centers[i][j].x - (240 + 110))*((256 + 360) - (0 + 360)) - (centers[i][j].y - (0 + 360)) * ((285 + 110) - (240 + 110));
			//double R2 = (centers[i][j].x - (290 + 110))*((256 + 360) - (0 + 360)) - (centers[i][j].y - (0 + 360)) * ((370 + 110) - (290 + 110));
			////////////////double L1 = (centers[i][j].x - 260) * (256) - (centers[i][j].y - 360) * (-80);
			////////////////double L2 = (centers[i][j].x - 310) * (256) - (centers[i][j].y - 360) * (-45);
			////////////////double R1 = (centers[i][j].x - 350) * (256) - (centers[i][j].y - 360) * (45);
			////////////////double R2 = (centers[i][j].x - 400) * (256) - (centers[i][j].y - 360) * (80);

			double L1 = (centers[i][j].x - (150 + search_left)) * ((256 + search_top) - (0 + search_top)) - (centers[i][j].y - (0 + search_top)) * ((70 + search_left) - (150 + search_left));
			double L2 = (centers[i][j].x - (200 + search_left)) * ((256 + search_top) - (0 + search_top)) - (centers[i][j].y - (0 + search_top)) * ((155 + search_left) - (200 + search_left));
			double R1 = (centers[i][j].x - (240 + search_left)) * ((256 + search_top) - (0 + search_top)) - (centers[i][j].y - (0 + search_top)) * ((285 + search_left) - (240 + search_left));
			double R2 = (centers[i][j].x - (290 + search_left)) * ((256 + search_top) - (0 + search_top)) - (centers[i][j].y - (0 + search_top)) * ((370 + search_left) - (290 + search_left));

			if (L1 < 0) {
				lineas[0].push_back(centers[i][j]); //lineas[0]=left 1
			}
			else if ((L1 > 0) && (L2 < 0)) {
				lineas[1].push_back(centers[i][j]); //lineas[1]=left 2
			}
			else if ((L2 > 0) && (R1 < 0)) {
				lineas[2].push_back(centers[i][j]); //lineas[2]=center
			}
			else if ((R1 > 0) && (R2 < 0)) {
				lineas[3].push_back(centers[i][j]); //lineas[3]=right 1
			}
			else if (R2 > 0) {
				lineas[4].push_back(centers[i][j]); //lineas[0]=right 2
			}
		}
	}
}

// Extend the calculated lines to the ROI limits y=360 to y=616
void CamNavigator::linearizer(Point2f& A, Point2f& B, Point2f& A0, Point2f& B0)
{
	// Calculate line slope
	float m = (B.y - A.y) / (B.x - A.x);
	// Calculate line intercept
	float b = A.y - m * A.x;
	// Calculate line's end points
	//A0.x = (360 - b) / m;
	//A0.y = 360;
	//B0.x = (616 - b) / m;
	//B0.y = 616;

	A0.x = (search_top - b) / m;
	A0.y = search_top;
	B0.x = (img_height - b) / m;
	B0.y = img_height;
}