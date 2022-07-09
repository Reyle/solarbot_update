#include "UdpVideoSender.h"


UdpVideoSender::UdpVideoSender(std::string UdpServerHost, uint16_t UdpServerPort, int UdpServerPacketSize, int Quality)
{
	this->UdpServerHost = UdpServerHost;
	this->UdpServerPort = UdpServerPort;
	this->UdpServerPacketSize = UdpServerPacketSize;

	tx_buffer = (unsigned char*)malloc(UdpServerPacketSize);

    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(Quality);
    compression_params.push_back(cv::IMWRITE_JPEG_PROGRESSIVE);
    compression_params.push_back(1);
    compression_params.push_back(cv::IMWRITE_JPEG_OPTIMIZE);
    compression_params.push_back(1);
    compression_params.push_back(cv::IMWRITE_JPEG_LUMA_QUALITY);
    compression_params.push_back(Quality);
}

void UdpVideoSender::SendImage(cv::Mat img)
{
    std::vector<uchar> encoded;
    cv::imencode(".jpg", img, encoded, compression_params);//Encode the Mat to jpeg

    header.FrameType = 0x01;
    header.FrameId++;
    header.PayloadLength = (int32_t)encoded.size();

    Send(header, encoded.data());
}

void UdpVideoSender::Send(Header header, unsigned char *payload)
{
    try
    {
        int data_index = 0;
        int pack_index = 0;
        char* pHeader = (char*)&header;
        //int length = header.PayloadLength;

        unsigned char a;
        for (int pack_index = 0; data_index < header.PayloadLength; pack_index++)
        {
            if (pack_index == 0)//Add the header in the first packet
            {
                data_index = 0;
                tx_buffer[0] = 0x7E; //Start Frame indicator
                tx_buffer_index = 1;

                //Add Header content to tx_buffer
                for (int i = 0; i < sizeof(Header); i++)
                {
                    a = *(pHeader + i);
                    if (a == 0x7E || a == 0x7D)
                    {
                        tx_buffer[tx_buffer_index] = 0x7D;
                        tx_buffer_index++;
                        tx_buffer[tx_buffer_index] = a ^ 0x20;
                    }
                    else
                    {
                        tx_buffer[tx_buffer_index] = a;
                    }
                    tx_buffer_index++;
                }
            }

            //Add payload (image) to tx_buffer
            while ((tx_buffer_index < UdpServerPacketSize - 6) && (data_index < header.PayloadLength))//to left space for escape sequence
            {
                a = payload[data_index];
                if (a == 0x7E || a == 0x7D)
                {
                    tx_buffer[tx_buffer_index] = 0x7D;
                    tx_buffer_index++;
                    tx_buffer[tx_buffer_index] = a ^ 0x20;
                }
                else
                {
                    tx_buffer[tx_buffer_index] = a;
                }
                tx_buffer_index++;
                data_index++;
            }

            if (data_index == header.PayloadLength)
            {
                uint16_t crc = crc16.calculate(payload, header.PayloadLength);

                unsigned char* pa = (unsigned char*)&crc;
                for (int i = 0; i < 2; i++)
                {
                    a = *pa;
                    pa++;
                    if (a == 0x7E || a == 0x7D)
                    {
                        tx_buffer[tx_buffer_index] = 0x7D;
                        tx_buffer_index++;
                        tx_buffer[tx_buffer_index] = a ^ 0x20;
                    }
                    else
                    {
                        tx_buffer[tx_buffer_index] = a;
                    }
                    tx_buffer_index++;
                }

                //printf("CRC %02X %02X\n", tx_buffer[tx_buffer_index - 2], tx_buffer[tx_buffer_index - 1]);
            }

            //Send to server
            sock.sendTo(tx_buffer, tx_buffer_index, UdpServerHost, UdpServerPort);
            //std::cout << tx_buffer_index << std::endl;
            tx_buffer_index = 0;
        }
    }
    catch (PracticalSocket::SocketException& e) {
        cerr << e.what() << endl;
    }

}