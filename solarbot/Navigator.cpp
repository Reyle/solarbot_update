#include "Navigator.h"
#include "err.h"
#include <stdio.h>
#include <stdlib.h>     /* strtold */
#include <iostream> // for standard I/O
#include <fstream>

Navigator::Navigator()
{
}

Navigator::~Navigator()
{

}

int Navigator::Init(ControlUnit* driver, Head* head)
{
	int result = 0;

	this->driver = driver;
	this->head = head;
	finding_path = false;

	return result;
}

int Navigator::LoadWaypoints(const char* file)
{
	/*
	Ejemplo de archivo de configuracion:

		#Camino de cuatro puntos frente a la empresa
		S1
		L
		C4
		P-21.220044,-50.410073,1000
		P-21.220133,-50.409763,1000
		P-21.219855,-50.409697,1000
		P-21.219766,-50.409943,1000

	Todas las lineas tienen que terminar en \r\n
	Cualquier linea que comience con el caracter # es un comentario
	la primera letra de la linea indica el tipo de registro:
		S indica el waypoint por el cual comenzar, si no está se comienza por el primer waypoint
		L indica que llegado al último waypoint se comience por el primero, sino está al llegar al final se para la navegacion
		C indica la cantidad de waypoints y tiene que ser el ultimo registro antes que comiencen los registros de waypoints
		  si despues del registro C aparece un registro que no sea un waypoint es ignorado. Igualmente si aparece un registro de waypoint antes del cmpo C es tambien ignorado
		P indica un waypoint que tiene los campos Latitud,Longitud,Velocidad. Tienen que existir la misma cantida de registros tipo P que los indicados en el valor del registro C

	*/

	printf("Reading waypoints...\n");

	try
	{
		std::ifstream ifile(file);
		if (ifile)
		{
			waypoints_index = -1;
			waypoints_size = 0;
			waypoints.clear();
			LoopNavigate = false;
			finding_path = true;

			int i = 0;
			int read = 0;
			int comma_count;
			int last_comma_index = 0;
			int start_waypoint = -1;

			double latitude = 0;
			double longitude = 0;
			int speed = 0;
			struct Waypoint point;


			string line;
			while (std::getline(ifile, line))
			{
				//std::cout << line << std::endl;
				read = line.length();
				if (read > 0)
				{
					if (line[0] == 'P')
					{
						last_comma_index = 0;
						comma_count = 0;
						for (i = 1; i < read; i++)
						{
							if (line[i] == ',' || i == (read -1))
							{
								comma_count++;

								if (comma_count == 1)
								{
									latitude = strtold(line.c_str() + last_comma_index + 1, NULL);
								}
								else if (comma_count == 2)
								{
									longitude = strtold(line.c_str() + last_comma_index + 1, NULL);
								}
								else if (comma_count == 3)
								{
									speed = atoi(line.c_str() + last_comma_index + 1);
									//printf("Rx Lat=%f  Lon=%f  Speed=%d\n", latitude, longitude, speed);
									point.latitude = latitude;
									point.longitude = longitude;
									point.speed = speed;
									waypoints.push_back(point);

									//printf("Rb Lat=%f  Lon=%f  Speed=%d\n", waypoints[waypoints_index].latitude, waypoints[waypoints_index].longitude, waypoints[waypoints_index].speed);

									//waypoints_index++;
								}

								last_comma_index = i;
							}
						}
					}
					else if (line[0] == 'S')
					{
						finding_path = false;
						start_waypoint = atoi(line.c_str() + 1);
						start_waypoint--;
						if (start_waypoint < 0)
							start_waypoint = 0;
					}
					else if (line[0] == 'L')
					{
						LoopNavigate = true;
					}

				}
			}
		}
		else
		{
			return err::ERR_NAVIGATOR_OPENING_FILE;
		}
	}
	catch(...)
	{
		return err::ERR_NAVIGATOR_INVALID_FILE_FORMAT;
	}

	waypoints_size = waypoints.size();

	printf("\t%d waypoints were read\n", waypoints_size);
	if (waypoints_index == -1)
		printf("\tstart by the closest waypoint\n");
	else
		printf("\tstart by waypoint %d\n", waypoints_index - 1);

	if (LoopNavigate)
		printf("\tloop navigation\n");
	else
		printf("\tstop when arrive to the last waypoint\n");

	return err::OK;
}

double Navigator::Degrees2Radians(double degree)
{
	return degree * M_PI / 180;
}

double Navigator::DistanceBetweenTwoPoint(Point p1, Point p2)
{
	double lat1 = Degrees2Radians(p1.lat);
	double long1 = Degrees2Radians(p1.lon);
	double lat2 = Degrees2Radians(p2.lat);
	double long2 = Degrees2Radians(p2.lon);

	return DistanceBetweenTwoPoint(lat1, long1, lat2, long2);
}

double Navigator::DistanceBetweenTwoPoint(double lat1, double long1, double lat2, double long2)
{
	double R = 6371000; //in meters
	double lat1r, lon1r, lat2r, lon2r, u, v;
	lat1r = lat1 * M_PI / 180;
	lon1r = long1 * M_PI / 180;
	lat2r = lat2 * M_PI / 180;
	lon2r = long2 * M_PI / 180;
	u = sin((lat2r - lat1r) / 2);
	v = sin((lon2r - lon1r) / 2);
	return 2.0 * R * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v));
}

double Navigator::GetBearing(Point from, Point to)
{
	double lat1 = Degrees2Radians(from.lat); //a
	double long1 = Degrees2Radians(from.lon); //b
	double lat2 = Degrees2Radians(to.lat); //c
	double long2 = Degrees2Radians(to.lon); //d

	if (cos(lat2) * sin(long2 - long1) == 0)
		if (lat2 > lat1)
			return 0;
		else
			return 180;
	else
	{
		double angle = atan2(cos(lat2) * sin(long2 - long1), sin(lat2) * cos(lat1) - sin(lat1) * cos(lat2) * cos(long2 - long1));
		return ((int)(angle * 180 / M_PI) + 360) % 360;
	}
}

Navigator::Destination Navigator::GetDestination(Point from, Point to)
{
	Destination destination;
	double lat1 = Degrees2Radians(from.lat);
	double long1 = Degrees2Radians(from.lon);
	double lat2 = Degrees2Radians(to.lat);
	double long2 = Degrees2Radians(to.lon);

	// Haversine Formula 
	double dlong = long2 - long1;
	double dlat = lat2 - lat1;

	double ans = pow(sin(dlat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dlong / 2), 2);

	ans = 2 * asin(sqrt(ans));

	// Radius of Earth in  
	// Kilometers, R = 6371 
	// Miles, R = 3956
	// Meters, R = 6371000
	double R = 6371000; //in meters

	// Calculate the result 
	destination.distance = ans * R;


	if (cos(lat2) * sin(long2 - long1) == 0)
		if (lat2 > lat1)
			destination.bearing = 0;
		else
			destination.bearing = 180;
	else
	{
		double angle = atan2(cos(lat2) * sin(long2 - long1), sin(lat2) * cos(lat1) - sin(lat1) * cos(lat2) * cos(long2 - long1));
		destination.bearing = ((int)(angle * 180 / M_PI) + 360) % 360;
	}


	return destination;
}

Navigator::Point Navigator::GetDestinationPoint(Point from, Destination destination)
{
	return GetDestinationPoint(from, destination.distance, destination.bearing);
}

Navigator::Point Navigator::GetDestinationPoint(Point from, double distance, double bearing)
{
	return GetDestinationPoint(from.lat, from.lon, distance, bearing);
}

Navigator::Point Navigator::GetDestinationPoint(double latitude, double longitude, double distance, double bearing)
{
	//TODO: There is a lot of optimization to do here

	double rr = distance / 6371000; //in meters
	double lat1 = latitude * M_PI / 180;
	double lon1 = longitude * M_PI / 180;
	double angle = bearing * M_PI / 180;

	double lat2 = asin(sin(lat1) * cos(rr) + cos(lat1) * sin(rr) * cos(angle));
	double lon2 = lon1 + atan2(sin(angle) * sin(rr) * cos(lat1), cos(rr) - sin(lat1) * sin(lat2));

	Point p;
	p.lat = lat2 * 180 / M_PI;
	p.lon = lon2 * 180 / M_PI;

	return p;
}



int Navigator::Move(int speed, double angle, double distance)
{
	int speed_dir_contrarias = 500; //Cuando una rueda se mueve para alante y la otra para atrás
	if (DoNotMove)
	{
		MotorLeft = 0;
		MotorRight = 0;

		if (!eyes_angry)
		{
			head->SetEyes(Head::E_ANGRY, true, true, false);
			eyes_angry = true;
		}

		eyes_left = false;
		eyes_right = false;
		fast_eyes_left = false;
		fast_eyes_right = false;
	}
	else
	{
		eyes_angry = false;
		int MinimalSpeed = speed / 2;
		double SteeringSpeed;
		double slow_Angle;
		
		if (Mode == Navigator::Modes::ModeVisual)
		{
			SteeringSpeed = 400;
			slow_Angle = 90;
		}
		else
		{
			if (distance < 1.0)
			{
				SteeringSpeed = 400;
				slow_Angle = 45;
			}
			else
			{
				SteeringSpeed = (double)(speed - MinimalSpeed);
				slow_Angle = 15;
			}
		}

		double slow_Angle_n = -slow_Angle;

		if (angle > 180)
		{
			angle -= 360;
		}
		else if (angle < -180)
		{
			angle += 360;
		}		

		if (angle > slow_Angle_n && angle < slow_Angle)
		{
			//Mesma direción con factor diferente
			if (angle > 0)
			{
				MotorLeft = speed;
				MotorRight = speed - (int)((angle / slow_Angle) * SteeringSpeed);

				if (!eyes_right)
				{
					eyes_right = true;
					head->SetEyes(Head::E_LOOK_R, false, true, false);
				}
				eyes_left = false;
			}
			else
			{
				MotorLeft = speed + (int)((angle / slow_Angle) * SteeringSpeed);
				MotorRight = speed;

				if (!eyes_left)
				{
					eyes_left = true;
					head->SetEyes(Head::E_LOOK_L, false, true, false);
				}

				eyes_right = false;
			}
			fast_eyes_left = false;
			fast_eyes_right = false;
		}
		else
		{
			//Direciones contrarias
			if (angle > 0)
			{
				MotorLeft = speed_dir_contrarias;
				MotorRight = -speed_dir_contrarias;
				if (!fast_eyes_right)
				{
					fast_eyes_right = true;
					head->SetEyes(Head::E_ARROW_RIGHT, false, true, false);
				}
				fast_eyes_left = false;
			}
			else
			{
				MotorLeft = -speed_dir_contrarias;
				MotorRight = speed_dir_contrarias;

				if (!fast_eyes_left)
				{
					fast_eyes_left = true;
					head->SetEyes(Head::E_ARROW_LEFT, false, true, false);
				}

				fast_eyes_right = false;
			}

			eyes_left = false;
			eyes_right = false;
		}		
	}

	return driver->Write(MotorLeft, MotorRight);
}

int Navigator::GotoWaypoint(GPS::Location location, double heading, Waypoint waypoint, double* remaining_distance)
{
	//int WorkSpeed = waypoint.speed;
	//int WorkMinimalSpeed = WorkSpeed/2;
	//double WorkSteeringSpeed = (double)(WorkSpeed - WorkMinimalSpeed);

	int result = 0;
	Point fromPoint, toPoint;
	

	fromPoint.lat = location.latitude;
	fromPoint.lon = location.longitude;

	toPoint.lat = waypoint.latitude;
	toPoint.lon = waypoint.longitude;

	Destination destination = GetDestination(fromPoint, toPoint);
	if (remaining_distance != nullptr)
		*remaining_distance = destination.distance;

	//TODO: VERIFY precision of detect the rover arrived to the point
	double dist_to_compare = 1.2 * last_distance;
	//std::cout << destination.distance << "\t" << dist_to_compare << std::endl;
	if (destination.distance < 0.1)
	{		
		result = 0;
		last_distance = 1.0;
	}
	else if (destination.distance < 1.0)
	{
		//double dist_to_compare = 1.2 * last_distance;
		double angle = abs(destination.bearing - heading);
		if (angle > 45)//to avoid oscillations when angles is bigger thna 45 and is closer than 1 meter decide that th waypoint was reached
		{
			result = 0;
		}
		else
		{
			if (destination.distance > dist_to_compare)
			{
				result = 0;
				last_distance = 1.0;
			}
			else
			{
				last_distance = destination.distance;
				result = 1;
			}
		}
	}
	else
	{
		last_distance = 1.0;
		result = 1;
	}

	if (result == 1)
	{
		double angle = destination.bearing - heading;
		Move(waypoint.speed, angle, destination.distance);
	}

	return result;

}

int Navigator::GotoNextWaypoint(GPS::Location location, double heading, double* remaining_distance)
{
		int r;

		if (finding_path)
		{
			if (waypoints_index == -1)
			{
				//start_waypoint_index
				Point start_point = FindClosestPathPoint(location, heading, &waypoints_index);
				start_waypoint.latitude = start_point.lat;
				start_waypoint.longitude = start_point.lon;
				start_waypoint.speed = 1000;				
			}

			printf("Start point lat=%f\tlon=%f\n", start_waypoint.latitude, start_waypoint.longitude);

			
			double d = DistanceBetweenTwoPoint(location.latitude, location.longitude, start_waypoint.latitude, start_waypoint.longitude);
			//printf("Dist to start %f\n", d);
			if (d < 3.0)
				start_waypoint.speed = 300;

			r = GotoWaypoint(location, heading, start_waypoint, remaining_distance);
			if (r == 0)
			{
				finding_path = false; //in case r is zero the rover reached the closest point of the path
			}
		}
		else
		{
			r = GotoWaypoint(location, heading, waypoints[waypoints_index], remaining_distance);
			if (r == 0)
			{
				waypoints_index++;
				if (waypoints_index >= waypoints_size)
				{
					if (LoopNavigate)
					{
						waypoints_index = 0;
					}
					else
					{
						waypoints_index--;
						return err::NAVIGATOR_DONE;
					}
				}
			}
		}		

		return 0;
}

Navigator::Waypoint Navigator::GetCurrentWaypoint()
{
	if (waypoints_index > -1)
	{
		return waypoints[waypoints_index];
	}
	else
	{
		Waypoint empty_waypoint;
		empty_waypoint.latitude = 0;
		empty_waypoint.longitude = 0;
		empty_waypoint.speed = 0;
		return empty_waypoint;
	}
}

Navigator::Point Navigator::FindClosestPathPoint(GPS::Location location, double heading, int* start_waypoint_index)
{
	Point closest_point;
	closest_point.lat = location.latitude; //just in case
	closest_point.lon = location.longitude; //just in case

	double closest_distance = DBL_MAX;
	double closest_distance_to_next_waypoint = DBL_MAX;
	int next_waypoint_index;
	double increment = 1.0; //in meters
	Point p, p1, p2;
	double bearing;
	Destination dest;
	double d, dn;

	Point my_point;
	my_point.lat = location.latitude;
	my_point.lon = location.longitude;
	
	Point org_waypoint;
	Point dest_waypoint;

	for (int index = 0; index < waypoints_size; index++)
	{
		closest_distance_to_next_waypoint = DBL_MAX;

		next_waypoint_index = index + 1;
		if (next_waypoint_index >= waypoints_size)
			next_waypoint_index = 0;
		
		p1.lat = waypoints[index].latitude;
		p1.lon = waypoints[index].longitude;

		p2.lat = waypoints[next_waypoint_index].latitude;
		p2.lon = waypoints[next_waypoint_index].longitude;

		dest = GetDestination(p1, p2);

		bearing = dest.bearing; //The bearing from the current waypoit to the next one.
		d = dest.distance; //The distance between the current waypoint to the next.

		int steps = (d / increment); //Calculate the number of points in the path 
		org_waypoint.lat = waypoints[index].latitude;
		org_waypoint.lon = waypoints[index].longitude;

		dest_waypoint.lat = waypoints[next_waypoint_index].latitude;
		dest_waypoint.lon = waypoints[next_waypoint_index].longitude;


		Point destination_point;
		for (int i = 1; i < steps; i++)
		{
			destination_point = GetDestinationPoint(org_waypoint, i * increment, bearing); //Calculate the next point in the path
			d = DistanceBetweenTwoPoint(my_point, destination_point);
			
			if (d < closest_distance)
			{	
				dn = DistanceBetweenTwoPoint(destination_point, dest_waypoint);
				if (dn < closest_distance_to_next_waypoint)
				{
					closest_distance_to_next_waypoint = dn;
					closest_distance = d;
					closest_point = destination_point; // GetDestinationPoint(destination_point, 1.2, bearing);
					*start_waypoint_index = next_waypoint_index;
				}
			}
		}
	}



	return closest_point;
}

CamNavigator::Result Navigator::GotoNextCam(Mat mat_bgr)
{
	CamNavigator::Result r = cam.ProcessImage(mat_bgr);
	int speed = r.quality * CamWorkSpeed/ 100.0;
	Move(speed, r.angle);

	char buff[255];
	sprintf(buff, "A=%.1f  Q=%d  U=%d  L=%d  R=%d  S=%d", r.angle, r.quality, r.used_row, MotorLeft, MotorRight, speed);
	printf("%s\n", buff);

	if (cam.SHOW_CAM_NAVIGATOR || cam.SAVE_CAM_NAVIGATOR)
	{		
		cam.AddDebugInformationToImage(std::string(buff));
	}

	if (cam.SAVE_CAM_NAVIGATOR)
	{
		//printf("Saving dsp. %d, %d\n", cam.dsp.cols, cam.dsp.rows);
		cam.video_writer.write(cam.dsp);
	}

	if (cam.SHOW_CAM_NAVIGATOR)
	{
		
		//cv::imshow("FINAL", cam.dsp);
		////cv::imshow("debug", cam.debug_mat);
		//if (waitKey(1) == 27)
		//{

		//}

		//Send Image
		cam.SendDebugImg2UDPServer();
	}

	return r;
}


int Navigator::GotoByLine()
{
	int r = line.Read(&line_data);
	if (r == 0)
	{
		Move(LineWorkSpeed, line_data.angle);
		return 0;
	}
	else
	{
		line.Close();
		line.Open();
		return r;
	}

}


Navigator::Modes Navigator::String2Mode(std::string s_mode)
{
	if (s_mode == "line")
	{
		return Navigator::Modes::ModeLine;
	}
	else if (s_mode == "visual")
	{
		return Navigator::Modes::ModeVisual;
	}
	else if (s_mode == "waypoints")
	{
		return Navigator::Modes::ModeWaypoints;
	}
	else
	{
		return Navigator::Modes::Standby;
	}
}
