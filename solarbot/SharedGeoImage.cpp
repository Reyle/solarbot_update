
#include "SharedGeoImage.h"
#include "boost/date_time/posix_time/posix_time.hpp"


int SharedGeoImage::Open(std::string name, int size, bool sender)
{
    this->name = name;

    if (sender)
        shared_memory_object::remove(name.c_str());

    this->sender = sender;
    int result = 0;    

    try
    {
        //Create a shared memory object.
        if (sender)
        {
            boost::interprocess::permissions permissions;
            permissions.set_unrestricted();
            shm = shared_memory_object(create_only, name.c_str(), read_write, permissions);
        }
        else
        {
            shm = shared_memory_object(open_only, name.c_str(), read_write);
        }

        //Set size
        this->size = size;
        shm.truncate(sizeof(shared_memory_header) + size);

        //Map the whole shared memory in this process
        region = mapped_region(shm, read_write);

        //Get the address of the mapped region
        void* addr = region.get_address();

        if (sender)
        {
            //Construct the shared structure in memory
            shared_header = new (addr) shared_memory_header;
        }
        else
        {
            //Obtain the shared structure
            shared_header = static_cast<shared_memory_header*>(addr);
        }
    }
    catch(...)
    {
        result = -1;
    }
    return result;
}

int SharedGeoImage::Write(double latitude, double longitude, double heading, unsigned char* buff)
{
    shared_header->latitude = latitude;
    shared_header->longitude = longitude;
    shared_header->heading = heading;
    shared_header->IdFrame++;
    memcpy(region.get_address() + sizeof(shared_memory_header), buff, size);
    shared_header->mutex.post();

    return 0;
}

int SharedGeoImage::Read(double* latitude, double* longitude, double* heading, unsigned char* buff, unsigned char* IdFrame)
{
    boost::posix_time::ptime until = boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(1);
    bool result = shared_header->mutex.timed_wait(until);

    if (!result)
        return -2;

    *latitude = shared_header->latitude;
    *longitude = shared_header->longitude;
    *heading = shared_header->heading;
    if (IdFrame != nullptr)
        *IdFrame = shared_header->IdFrame;
    memcpy(buff, region.get_address() + sizeof(shared_memory_header), size);

    return 0;
}

void SharedGeoImage::Close()
{
    if (sender)
        shared_memory_object::remove(name.c_str());
}
