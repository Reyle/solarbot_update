#pragma once
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>


using namespace boost::interprocess;

class SharedGeoImage
{
public:
	int Open(std::string name, int size, bool sender = false);
	int Write(double latitude, double longitude, double heading, unsigned char* buff);
	int Read(double* latitude, double* longitude, double* heading, unsigned char* buff, unsigned char* IdFrame = nullptr);
	void Close();

	

private:

	struct shared_memory_header
	{		
		shared_memory_header() : mutex(1)
		{}
		
		boost::interprocess::interprocess_semaphore	mutex; //Semaphore to synchronize access
		double latitude;
		double longitude;
		double heading;
		unsigned char IdFrame;
	};

	shared_memory_object shm;
	mapped_region region;
	int size = -1;
	shared_memory_header* shared_header;
	bool sender;
	std::string name = "";
};

