#pragma once

#include <math.h>
#include <string.h>

#include "GPS.h"
#include "ControlUnit.h"
#include "Head.h"
#include "CamNavigator.h"
#include "LineNavigator.h"


class Navigator
{
public:
	Navigator();
	~Navigator();


	struct Point
	{
		double lat;
		double lon;
	};

	///Bearing and Distance to a point
	struct Destination 
	{
		double bearing;
		double distance;
	};

	typedef struct Waypoint
	{
		double latitude;
		double longitude;
		int speed;
	};

	enum Modes
	{
		Standby, ModeWaypoints, ModeVisual, ModeLine
	};

	Head* head;
	Modes Mode = ModeWaypoints;
	ControlUnit* driver;
	CamNavigator cam;
	LineNavigator line;
	LineNavigator::Data line_data;
	bool DoNotMove = false;
	int MotorLeft = 0;
	int MotorRight = 0;
	int Init(ControlUnit* _driver, Head* head);
	int LoadWaypoints(const char* file);

	//struct Waypoint* waypoints = NULL;
	std::vector<Waypoint> waypoints;
	int waypoints_size = 0;
	int waypoints_index = -1;
	bool LoopNavigate = false;
	double last_distance = 1.0;
	Waypoint start_waypoint; //start point to reach at the begining, it is the closest point of the path
	int start_waypoint_index; //Index of the waypoint to go to when the start_waypoint is reached
	bool finding_path = false;// Indicate that the rover is still going to the closest point of the path

	int CamWorkSpeed = 1000;
	int LineWorkSpeed = 1000;

	bool eyes_right = false;
	bool eyes_left = false;
	bool fast_eyes_right = false;
	bool fast_eyes_left = false;
	bool eyes_angry = false;

	///Convert Degrees to Radians
	double Degrees2Radians(double degree);

	///Calculate in meters the dinstance between two points
	double DistanceBetweenTwoPoint(Point p1, Point p2);
	double DistanceBetweenTwoPoint(double lat1, double long1, double lat2, double long2);

	///Calculate the Bearing from a point to other
	double GetBearing(Point p1, Point p2);

	///Return the Destination (bearing and distance) from one point to other.
	Destination GetDestination(Point from, Point to);

	//Return the Destination Point (latitude and longitude) from a point to a destination (bearing and distance)
	Point GetDestinationPoint(Point from, Destination destination);
	Point GetDestinationPoint(Point from, double distance, double bearing);
	Point GetDestinationPoint(double latitude, double longitude, double distance, double bearing);

	//Move the rover
	//speed in internal unit
	//angle in degree, a positive angle the rover will turn to the right, negative to the left
	//distance is to decide the correct angle bigger than it the wheels will move in oppositive directions
	int Move(int speed, double angle, double distance = 5.0);

	int GotoWaypoint(GPS::Location location, double heading, Waypoint waypoint, double* remaining_distance = nullptr);

	int GotoNextWaypoint(GPS::Location location, double heading, double* remaining_distance = nullptr);

	Waypoint GetCurrentWaypoint();

	//Find the closest point to path defined by the waypoints
	Point FindClosestPathPoint(GPS::Location location, double heading, int* start_waypoint_index);

	CamNavigator::Result GotoNextCam(Mat mat_bgr);

	int GotoByLine();

	Modes String2Mode(std::string s_mode);
};

