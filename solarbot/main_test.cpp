
#include <cstdio>
#include <iostream>
#include <string>
#include <signal.h>
#include <unistd.h>
#include "Navigator.h"
#include "PowerUnit.h"

using namespace std;

int main_test()
{
	/*Navigator navigator;
	int result = navigator.LoadWaypoints("/home/solarbot/waypoints.txt");

	printf("result=%d\n", result);

	printf("Count=%d\n", navigator.waypoints_size);
	printf("loop=%d\n", navigator.LoopNavigate);

	for(int i=0; i<navigator.waypoints_size; i++)
		printf("Lat=%f  Lon=%f  Speed=%d\n", navigator.waypoints[i].latitude, navigator.waypoints[i].longitude, navigator.waypoints[i].speed);*/


	printf("Opening Power Unit...");
	std::cout.flush();
	PowerUnit* power_unit = new PowerUnit();
	int r = power_unit->Open("/dev/ttyPowerUnit", 115200);
	//sleep(2);
	if (r < 0)
	{
		printf("ERROR\n");
		return -1;
	}
	else
	{
		printf("OK\n");
	}
	PowerUnit::PowerUnitData power_unit_data;
	int power_unit_status;
	while (1)
	{
		power_unit_status = power_unit->Read(&power_unit_data); //Get power unit data
		printf("Status=%d  Panel=%fV %dW  Batt=%fV %dW   E=%d  L=%d  R=%d  A=%d Stop=%d\n", power_unit_status, 
			power_unit_data.PanelVoltage, 
			power_unit_data.PanelPower, 
			power_unit_data.BatteryVoltage, 
			power_unit_data.BatteryPower,
			power_unit_data.ElectronicPower,
			power_unit_data.LeftPower,
			power_unit_data.RightPower,
			power_unit_data.AccessoriesPower,
			power_unit_data.StopPressed
		);
		std::cout.flush();

		if (power_unit_status != 0)
		{
			power_unit->Close();
			power_unit->Open("/dev/ttyPowerUnit", 115200);
			printf("RESETING!\n");
		}


		usleep(200000);
	}



}