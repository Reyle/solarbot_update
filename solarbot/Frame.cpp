#include "Frame.h"
#include <iostream>
#include <vector>

//class EscapedVector :std::vector<unsigned char>
//{
//public:
//    void add_with_escape(unsigned char b)
//    {
//        if (b == 0x7E || b == 0x7D)
//        {
//            this->push_back(0x7D);
//            this->push_back((unsigned char)(b ^ 0x20));
//        }
//        else
//        {
//            this->push_back(b);
//        }
//    }
//};

bool Frame::DecodeFrame(unsigned char a)
{
    int result = false;

    if (a == 0x7E)
    {
        frame_buffer_index = 0;        
        rx_PayloadLength = 0;
        decoding = true;
    }
    else if (decoding)
    {
        if (a == 0x7D)
        {
            escaping = true;
        }
        else
        {
            if (escaping)
            {
                a ^= 0x20;
                escaping = false;
            }

            if (frame_buffer_index < frame_buffer_size) //To avoid Overflow
            {
                frame_buffer[frame_buffer_index] = a;                

                if (frame_buffer_index == 7) //We have the header, lets decode and verify its CRC16
                {
                    Header* p_rx_header = (Header*)&frame_buffer;
                    if (p_rx_header->HeaderCRC == crc.calculate(frame_buffer, 6))
                    {
                        rx_FrameType = p_rx_header->FrameType;
                        rx_FrameId = p_rx_header->FrameId;
                        rx_PayloadLength = p_rx_header->PayloadLength;
                        //printf("FrameType=%u\tFrameId=%u\tPayloadLength=%u\n", rx_FrameType, rx_FrameId, rx_PayloadLength);
                    }
                    else
                    {
                        decoding = false;
                        std::cerr << "ERROR: Invalid Header CRC" << std::endl;
                    }
                }
                else if (frame_buffer_index > 7) //We are in the payload
                {
                    if (frame_buffer_index == rx_PayloadLength + 9)//If reach the end verify the payload CRC16
                    {
                        uint16_t rx_crc = *(uint16_t*)(frame_buffer + 8 + (int)rx_PayloadLength);
                        uint16_t calculate_crc = crc.calculate(frame_buffer + 8, rx_PayloadLength);
                        if (calculate_crc == rx_crc)
                        {
                            result = true;
                            decoding = false;
                        }
                        else
                        {
                            result = false;
                            std::cerr << "ERROR: Invalid Payload CRC" << std::endl;
                        }
                    }
                }

                frame_buffer_index++;
            }
            else
            {
                decoding = false;
                std::cerr << "ERROR: Overflow" << std::endl;
            }
        }
    }


    return result;
}

int Frame::GenerateFrame(unsigned char* buffer, int buffer_size, uint16_t FrameType, uint16_t FrameId, unsigned char* Payload, uint16_t PayloadLength)
{
    if (buffer_size < 2*(10 + PayloadLength))//In the worse case each byte will be converted in two bytes
        return 0;

    int buffer_index;

    CRC16 crc16;

    //Generate random byte with value diferent than 0x7E
    unsigned char rnd_byte = 0x7E;
    while (rnd_byte == 0x7E)
        rnd_byte = (unsigned char)(rand() % (256));



    //Create header
    Header header;
    header.FrameType = FrameType;
    header.FrameId = FrameId;
    header.PayloadLength = PayloadLength;

    //Get the first part of the header bytes array
    unsigned char header_bytes[8];
    unsigned char* p = (unsigned char*)&header;
    for (int i = 0; i < 6; i++)
    {
        header_bytes[i] = *p;
        p++;
    }

    //Calculate the Header CRC16
    header.HeaderCRC = crc16.calculate(header_bytes, 6);

    //Add the rest of the header(CRC16) to the header bytes array
    header_bytes[6] = *p;
    p++;
    header_bytes[7] = *p;


    buffer[0] = rnd_byte; //First byte in the frame is random to improve the encription security
    buffer[1] = 0x7E; //Frame start
    buffer_index = 2;

    //Add the header
    for (int i = 0; i < 8; i++)
        AddWithEscape(buffer, &buffer_index, header_bytes[i]);

    //Add the payload
    for (int i = 0; i < PayloadLength; i++)
        AddWithEscape(buffer, &buffer_index, Payload[i]);

    //calculate the payload CRC16
    uint16_t payload_crc16 = crc16.calculate(Payload, PayloadLength);

    //Add the payload CRC16
    p = (unsigned char*)&payload_crc16;
    AddWithEscape(buffer, &buffer_index, *p);
    p++;
    AddWithEscape(buffer, &buffer_index, *p);

    //Return the frame length
    return buffer_index;
}

int Frame::GeneratePingACKFrame(unsigned char* buffer, int buffer_size, uint16_t IdFrame)
{
    return GenerateFrame(buffer, buffer_size, FramesTypes::PingACK, IdFrame, NULL, 0);
}

int Frame::GenerateCommandACKFrame(unsigned char* buffer, int buffer_size, uint16_t IdFrame, std::string msg)
{
    return GenerateFrame(buffer, buffer_size, FramesTypes::CommandACK, IdFrame, (unsigned char*)(msg.c_str()), msg.length());
}

void Frame::AddWithEscape(unsigned char* buffer, int* buffer_index, unsigned char b)
{
    if (b == 0x7E || b == 0x7D)
    {
        buffer[*buffer_index]  = 0x7D;
        (*buffer_index)++;

        buffer[*buffer_index] = (unsigned char)(b ^ 0x20);
        (*buffer_index)++;
    }
    else
    {
        buffer[*buffer_index] = b;
        (*buffer_index)++;
    }
}
