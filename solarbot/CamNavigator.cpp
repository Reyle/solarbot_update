#include <iostream> // for standard I/O
#include <fstream>

#include "CamNavigator.h"

//#include "unistd.h"



CamNavigator::CamNavigator()
{
	vector <Mat> temp_strips(16);
	strips = temp_strips;

	vector<vector<Point2f>> temp_centers(16);
	centers = temp_centers;


	//tr_left[0][0] = Point(0, 0);
	//tr_left[0][1] = Point(0, 192);
	//tr_left[0][2] = Point(105, 0);

	//ppt_tr_left[0] = tr_left[0];
	//npt_tr_left[0] = 3;

	//tr_right[0][0] = Point(420, 0);
	//tr_right[0][1] = Point(420, 192);
	//tr_right[0][2] = Point(315, 0);

	//ppt_tr_right[0] = tr_right[0];
	//npt_tr_right[0] = 3;

	//the element chosen here is a 11px by 22px rectangle
	//erodeElement = getStructuringElement(MORPH_RECT, Size(11, 22)); //Size(11, 11));
	erodeElement = getStructuringElement(MORPH_RECT, Size(14, 25)); //Size(11, 11));

	//dilate with larger element so make sure object is nicely visible
	//dilateElement = getStructuringElement(MORPH_RECT, Size(7, 14)); //Size(7, 7));
	dilateElement = getStructuringElement(MORPH_RECT, Size(10, 20)); //Size(7, 7));
}

CamNavigator::~CamNavigator()
{
	delete align_to_color;
	capture.release();
	capture.~VideoCapture();

	if (SHOW_CAM_NAVIGATOR)
		delete udp_video_sender;
}


bool CamNavigator::Init(int capture_width, int capture_height)
{
	this->capture_width = capture_width;
	this->capture_height = capture_height;

	//Calculate the resulting image after zoomed
	img_width = img_zoom * capture_width;
	img_height = img_zoom * capture_height;

	if (img_width < search_width)
	{
		std::cerr << "ERROR: CamNavigator::Init() The img_width cannot be less than search_width" << std::endl;
		return false;
	}

	if (img_height < search_height)
	{
		std::cerr << "ERROR: CamNavigator::Init() The img_height cannot be less than search_height" << std::endl;
		return false;
	}

	search_top = img_height - search_height - vertical_offset;
	if (search_top < 0)
	{
		std::cerr << "ERROR: CamNavigator::Init() The search_top cannot be negative" << std::endl;
		return false;
	}

	search_left = (img_width / 2) - (search_width / 2) + horizontal_offset;
	if (search_left < 0)
	{
		std::cerr << "ERROR: CamNavigator::Init() The search_left cannot be negative" << std::endl;
		return false;
	}

	if ((search_left + search_width) > img_width)
	{
		std::cerr << "ERROR: CamNavigator::Init() The (search_left + search_width) cannot be bigger than img_width" << std::endl;
		return false;
	}


	interest_width = 100;
	interest_height = 50;
	interest_left = (img_width / 2) - (interest_width / 2) + horizontal_offset;
	interest_top = img_height - interest_height - vertical_offset;


	search_rect = Rect(search_left, search_top, search_width, search_height);
	interest_rect = Rect(interest_left, interest_top, interest_width, interest_height);

	// ***************  Fill the row_lines vector ***************
	vector<Point2f> temp_line;
	temp_line.push_back(Point(0, 0));
	temp_line.push_back(Point(0, 0));
	for (int i = 0; i < rows; i++)
	{
		row_lines.push_back(temp_line);
	}
	// ***************  END Fill the row_lines vector ***************


	// ***************  Fill the separation_lines vector ***************
	//ATTENTION: The separation lines need to be ordered from left to right!!!

	has_center_row = ((rows % 2) != 0); //if rows is odd then there is a center row

	int i_start, i_stop;
	int xm = search_width / 2;
	int half_width = row_width / 2;
	int x;


	if (has_center_row)
	{
		i_stop = (rows - 1) / 2;
		i_start = -i_stop;
		for (int i = i_start; i < i_stop; i++)
		{
			x = (i * row_width) + half_width;

			temp_line[0] = Point(xm + x, 0);
			temp_line[1] = Point(xm + x, search_height);
			separation_lines.push_back(temp_line);
		}
	}
	else
	{
		i_stop = (rows - 2) / 2;
		i_start = -i_stop;
		for (int i = i_start; i <= i_stop; i++)
		{
			x = (i * row_width);
			temp_line[0] = Point(xm + x, 0);
			temp_line[1] = Point(xm + x, search_height);
			separation_lines.push_back(temp_line);
		}
	}
	// ***************  END Fill the separation_lines vector ***************


	//Calculate the perspective transfor matrix
	Point2f pts1[] = { {(float)perspective_diff, 0} ,{(float)search_width - (float)perspective_diff, 0},{0, (float)search_height},{(float)search_width, (float)search_height} };
	Point2f pts2[] = { {0, 0},{(float)search_width, 0},{0, (float)search_height},{(float)search_width, (float)search_height} };
	PerspectiveTransformMatrix = getPerspectiveTransform(pts1, pts2);



	if (DEBUG_CAM_NAVIGATOR)
	{
		debug_mat = Mat(search_height, search_width, CV_8UC3);
		InvPerspectiveTransformMatrix = getPerspectiveTransform(pts2, pts1); //Inverted perspective transform matrix
	}

	if (SAVE_CAM_NAVIGATOR)
	{
		std::string full_path = "/home/solarbot/videos/" + save_video_filename;
		int i = 1;
		if (fexists(full_path))
		{
			do
			{
				full_path = "/home/solarbot/videos/" + to_string(i) + "-" + save_video_filename;
				i++;

			} while (fexists(full_path));
		}

		//video_writer.open(full_path, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 10, Size(img_width, img_height), true);
		video_writer.open(full_path, cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), 10, Size(img_width, img_height), true);

		if (!video_writer.isOpened())
		{
			cout << "Could not open the output video for write." << endl;
			return false;
		}


		//video_writer = VideoWriter(full_path, cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), 10, Size(dsp.cols, dsp.rows), true);

		cout << "Saving video to file " << full_path << endl;
	}

	if (SHOW_CAM_NAVIGATOR)
	{
		//// Filling address information for A
		//memset(&debug_udp_tx_address, 0, sizeof(debug_udp_tx_address));
		//debug_udp_tx_address.sin_family = AF_INET; // IPv4 
		//inet_pton(AF_INET, "172.16.1.60", &(debug_udp_tx_address.sin_addr));
		//debug_udp_tx_address.sin_port = htons(3003);

		//// Creating socket A file descriptor 
		//if ((debug_udp_tx_socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		//	perror("Socket for debug_udp_tx creation failed");
		//	return false;
		//}

		//Start video sender
		udp_video_sender = new UdpVideoSender("172.16.1.60", 3003);
	}


	return true;
}

CamNavigator::Result CamNavigator::ProcessImage(Mat pdsp)
{
	CamNavigator::Result result;
	result.readOK = true;// just for compatibility
	
	if (result.readOK)
	{
		if (DEBUG_CAM_NAVIGATOR)
			debug_mat = 0; //Clear the debug_mat here because some preccess may write on it

		//Resize imagen and save into dsp
		resize(pdsp, dsp, Size(), img_zoom, img_zoom, INTER_LANCZOS4);

		// Select search region
		search_region = dsp.clone()(search_rect);

		//Transform the perspective of the search region to look as view from above.
		cv::warpPerspective(search_region, search_region, PerspectiveTransformMatrix, Size(search_width, search_height));

		//Blur the search area
		cv::blur(search_region, search_region, Size(5, 5));

		// Convert frame from BGR to HSV colorspace
		cv::cvtColor(search_region, HSV, COLOR_BGR2HSV);

		// Filter HSV image between values and store filtered image to threshold matrix
		cv::inRange(HSV, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), threshld);


		//Draw separation lines. To be sure we will find some contounrs in the strips
		for (int i = 0; i < separation_lines.size(); i++)
			line(threshld, separation_lines[i][0], separation_lines[i][1], Scalar(0, 0, 0), 16);


		// Go to "dilate" and "erode" image function.
		//morphOps(threshld);


		// Split ROI into 16 subregions of 420x16 pixels
		for (int i = 0; i < strips.size(); i++) {
			strips[i] = threshld(Rect(0, 16 * i, 420, 16));

			// Find centers into the ROI's 16 subregions of 420x16 pixels
			centers[i] = findCenters(strips[i], Point(0, 16 * i));
		}

		//Separate center for each row line
		vector<vector<Point2f>> separatedCenters(row_lines.size());
		separator(centers, separatedCenters);

		//Save in row_lines the found line for each row
		getRowLines(separatedCenters);

		//*********************** Select the best row ********************
		int x_center_reference;
		int center_row_index;
		if (has_center_row)
		{
			center_row_index = (rows - 1) / 2;
			if (separatedCenters[center_row_index].size() > 4)
			{
				//Use center line
				result.used_row = center_row_index;
				x_center_reference = search_width / 2;
			}
			else
			{
				if (separatedCenters[center_row_index - 1].size() > separatedCenters[center_row_index + 1].size())
				{
					//Use left line
					result.used_row = center_row_index - 1;
					x_center_reference = (search_width / 2) - row_width;
				}
				else
				{
					//Use right line
					result.used_row = center_row_index + 1;
					x_center_reference = (search_width / 2) + row_width;
				}
			}
		}
		else
		{
			center_row_index = (rows - 2) / 2;
			if (separatedCenters[center_row_index].size() > separatedCenters[center_row_index + 1].size())
			{
				//Use left line
				result.used_row = center_row_index;
				x_center_reference = (search_width / 2) - (row_width/2);
			}
			else
			{
				//Use right line
				result.used_row = center_row_index + 1;
				x_center_reference = (search_width / 2) + (row_width/2);
			}
		}
		//******************** END Select the best row ********************


		//Calculate Quality
		float quality_factor = 6.25f;
		int points_count = separatedCenters[result.used_row].size();
		if (points_count > 16)
			points_count = 16;
		result.quality = quality_factor * points_count;		

		//Finally calculate the correction angle
		int x_row = row_lines[result.used_row][1].x;
		result.angle = navigation_angle_factor *(x_row - x_center_reference)/(row_width/2);

		//Limit the correction angle 
		if (result.angle > 80)
			result.angle = 80;
		if (result.angle < -80)
			result.angle = -80;
				

		if (DEBUG_CAM_NAVIGATOR)
		{
			//Draw centers
			Scalar color_blue = Scalar(255, 0, 0);
			Scalar color_mangeta = Scalar(255, 0, 255);
			Scalar color = color_blue;
			for (int g = 0; g < separatedCenters.size(); g++)
			{
				if (color == color_blue)
					color = color_mangeta;
				else
					color = color_blue;

				for (int i = 0; i < separatedCenters[g].size(); i++)
				{
					circle(debug_mat, separatedCenters[g][i], 4, color, -1, 8, 0);
				}
			}

			//Draw row_lines
			for (int i = 0; i < row_lines.size(); i++)
			{
				if (i == result.used_row)
				{
					line(debug_mat, row_lines[i][0], row_lines[i][1], Scalar(0, 255, 255), 2, LINE_AA);
				}
				else
				{
					line(debug_mat, row_lines[i][0], row_lines[i][1], Scalar(255, 0, 0), 2, LINE_AA);
				}
			}			

			//Draw separation lines.
			for (int i = 0; i < separation_lines.size(); i++)
				line(debug_mat, separation_lines[i][0], separation_lines[i][1], Scalar(80, 80, 100), 4);

			//Invert pespective
			cv::warpPerspective(debug_mat, debug_mat, InvPerspectiveTransformMatrix, Size(search_width, search_height));

			Mat roi_add = dsp(search_rect);
			cv::add(debug_mat, roi_add, roi_add);

			cv::rectangle(dsp, search_rect, Scalar(255, 0, 0), 1, 8, 0);
			cv::rectangle(dsp, interest_rect, Scalar(255, 0, 0), 1, 8, 0);

			//Draw the green rectangle that indicate the necesary correction
			//int auxcenter_x = search_left + (row_lines[result.used_row][0].x - (search_width / 2)) * 3 + (search_width / 2);
			int auxcenter_x = search_left + (search_width / 2) + (result.angle * (search_width / 2) / 80);
			cv::rectangle(dsp, Rect(auxcenter_x - 2, search_top + 10, 10, 5), Scalar(0, 255, 0), 8, LINE_AA);
		}
	}

	return result;
}

void CamNavigator::AddDebugInformationToImage(std::string msg)
{
	cv::rectangle(dsp, Point(0, 0), Point(dsp.cols, 15), Scalar(0, 0, 0), FILLED);
	cv::putText(dsp, msg, Point(5, 12), FONT_HERSHEY_PLAIN, 1, Scalar(255, 255, 255), 1, LINE_AA);
}

void CamNavigator::SendDebugImg2UDPServer()
{
	udp_video_sender->SendImage(dsp);
}

void CamNavigator::morphOps(Mat& thresh)
{
	//Opening Morphology Transformation: It is obtained by the erosion of an image	followed by a dilation.
	//create structuring element that will be used to "dilate" and "erode" image.
	// in this particular case, better results were obtained dilating first and then eroding
	dilate(thresh, thresh, dilateElement);
	erode(thresh, thresh, erodeElement);



} //OK

//Find the centers 
vector<Point2f> CamNavigator::findCenters(Mat roi, Point offs)
{
	//Note: The parameter "offs" is just for debug

	//these two vectors are needed for the output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;

	//find contours of filtered image using openCV findContours function
	findContours(roi, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

	// Get the moments
	vector<Moments> mu(contours.size());
	for (int i = 0; i < contours.size(); i++)
	{
		mu[i] = moments(contours[i], false);
	}

	// Get the mass centers:
	vector<Point2f> mc(contours.size());
	vector<Point2f> found_centers;
	for (int i = 0; i < contours.size(); i++)
	{ 
		mc[i] = Point2f((mu[i].m10 / mu[i].m00) + offs.x, (mu[i].m01 / mu[i].m00) + offs.y);

		// Check that the point is inside the ROI <--- Do I need to check??
		if (mc[i].x > 0 && mc[i].x < roi.cols && mc[i].y > offs.y && mc[i].y < (offs.y + roi.rows))
			found_centers.push_back(mc[i]);
	}

	if (DEBUG_CAM_NAVIGATOR)
	{
		// Draw contours
		for (int i = 0; i < contours.size(); i++)
		{
			drawContours(debug_mat, contours, i, Scalar(0, 0, 255), 2, 8, hierarchy, 0, offs);
		}

		for (int i = 0; i < found_centers.size(); i++)
		{
			circle(debug_mat, found_centers[i], 4, Scalar(255, 0, 0), -1, 8, 0);
		}
	}

	return found_centers;
} //OK

// Separate centers for each row
void CamNavigator::separator(vector<vector<Point2f>>& centers, vector<vector<Point2f>>& separatedCenters)
{
	int centers_i_count = centers.size();
	int centers_j_count;
	int r_count = rows - 1;
	bool find_place = false;
	for (int i = 0; i < centers_i_count; i++)
	{
		centers_j_count = centers[i].size();
		for (int j = 0; j < centers_j_count; j++)
		{
			for (int r = 0; r < r_count; r++)
			{
				find_place = false;
				if (centers[i][j].x < separation_lines[r][0].x)
				{
					separatedCenters[r].push_back(centers[i][j]);
					find_place = true;
					break;
				}				
			}

			if (!find_place)
				separatedCenters[r_count].push_back(centers[i][j]);
		}
	}



	//////************ TODO: Hacer que este código sea dinámico  ****************
	////vector<double> D(separation_lines.size());
	////for (int i = 0; i < centers.size(); i++) {
	////	for (int j = 0; j < centers[i].size(); j++) {
	////		// Evaluate if a point P(x,y) is on the left side or right side of the line AB; A(x1, y1) B(x2, y2)
	////		// d = (x-x1)*(y2-y1)-(y-y1)*(x2-x1)
	////		// if d<0 ->left side; if d>0 ->right side; if d=0 ->on the line

	////		for (int g = 0; g < D.size(); g++)
	////		{
	////			D[g] = ((centers[i][j].x - separation_lines[g][0].x) * (separation_lines[g][1].y - separation_lines[g][0].y)) - ((centers[i][j].y - separation_lines[g][0].y) * (separation_lines[g][1].x - separation_lines[g][0].x));
	////		}

	////		if (D[0] < 0) {
	////			separatedCenters[0].push_back(centers[i][j]); //lineas[0]=left 1
	////		}
	////		else if ((D[0] > 0) && (D[1] < 0)) {
	////			separatedCenters[1].push_back(centers[i][j]); //lineas[1]=left 2
	////		}
	////		else if ((D[1] > 0) && (D[2] < 0)) {
	////			separatedCenters[2].push_back(centers[i][j]); //lineas[2]=center
	////		}
	////		else if ((D[2] > 0) && (D[3] < 0)) {
	////			separatedCenters[3].push_back(centers[i][j]); //lineas[3]=right 1
	////		}
	////		else if (D[3] > 0) {
	////			separatedCenters[4].push_back(centers[i][j]); //lineas[0]=right 2
	////		}
	////	}
	////}
}

//Get the Row Lines
void CamNavigator::getRowLines(vector<vector<Point2f>>& separatedCenters)
{
	vector<Vec4f> fitlines(separatedCenters.size());
	Point2f line_p1;
	Point2f line_p2;
	int gap = 60; //to avoid float impresitions
	float vx;
	float vy;
	float x;
	float y;
	float lefty;
	float righty;


	for (int i = 0; i < fitlines.size(); i++) {
		if (separatedCenters[i].size() >= 2) {

			//fitlines = (vx, vy, x0, y0); (vx, vy) is a normalized vector collinear to the line and (x0, y0) is a point on the line
			fitLine(separatedCenters[i], fitlines[i], DIST_L2, 0, 1, 0.01);

			vx = fitlines[i][2] - gap * fitlines[i][0];
			vy = fitlines[i][3] - gap * fitlines[i][1];

			x = fitlines[i][2] + gap * fitlines[i][0];
			y = fitlines[i][3] + gap * fitlines[i][1];

			// Calculate line slope
			float md = (x - vx);
			if (md != 0)//avoid division by zero
			{
				float m = (y - vy) / md;

				if (m != 0)//avoid division by zero
				{
					// Calculate line intercept
					float b = vy - m * vx;

					// Calculate line's end points
					line_p1.x = (-b) / m;
					line_p1.y = 0;
					line_p2.x = (search_height - b) / m;
					line_p2.y = search_height;

					row_lines[i][0] = line_p1;
					row_lines[i][1] = line_p2;
				}
				else
				{
					row_lines[i][0] = Point(0, 0);
					row_lines[i][1] = Point(0, 0);
				}
			}
			else
			{
				row_lines[i][0] = Point(0, 0);
				row_lines[i][1] = Point(0, 0);
			}
		}
		else
		{
			row_lines[i][0] = Point(0, 0);
			row_lines[i][1] = Point(0, 0);
		}
	}
}


bool CamNavigator::fexists(const std::string& filename) 
{
	std::ifstream ifile(filename.c_str());
	return (bool)ifile;
}
