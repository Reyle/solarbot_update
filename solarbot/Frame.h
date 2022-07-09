#pragma once
#include <unistd.h>
#include <cinttypes>
#include <string>
#include "CRC16.h"

namespace FramesTypes
{
    enum FramesTypes { Ping, PingACK, Command, CommandACK };
}

class Frame
{
public:
#pragma pack(push, 1)
    typedef struct Header
    {
        uint16_t FrameType;
        uint16_t FrameId;
        uint16_t PayloadLength;
        uint16_t HeaderCRC;
    };
#pragma pack(pop)

    uint16_t rx_FrameType;
    uint16_t rx_FrameId;
    uint16_t rx_PayloadLength;

    const static int frame_buffer_size = 1024;
    unsigned char frame_buffer[frame_buffer_size];
    int frame_buffer_index = 0;

    bool DecodeFrame(unsigned char a);
    int GenerateFrame(unsigned char* buffer, int buffer_size, uint16_t FrameType, uint16_t FrameId, unsigned char* Payload, uint16_t PayloadLength);

    int GeneratePingACKFrame(unsigned char* buffer, int buffer_size, uint16_t IdFrame);
    int GenerateCommandACKFrame(unsigned char* buffer, int buffer_size, uint16_t IdFrame, std::string msg);


private:

    
    CRC16 crc;

    bool escaping = false;
    bool decoding = false;   

    /// <summary>
    /// Add value to the buffer and increment its index. In case value is equal to 0x7E or 0x7D, are escaped and the index in incremented bt two
    /// WARTING: The caller need to take care of there is at least two byte capacity available in the buffer
    /// </summary>
    /// <param name="buffer"></param>
    /// <param name="buffer_index"></param>
    /// <param name="value"></param>
    void AddWithEscape(unsigned char* buffer, int* buffer_index, unsigned char value);
    
};

