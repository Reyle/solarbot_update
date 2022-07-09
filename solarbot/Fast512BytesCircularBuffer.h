#pragma once

#pragma once
#include <cstdio>
#include <thread>
//#include <memory>
//#include <mutex>


/*
Fast 512 bytes circular buffer 
Eng. Deulis Antonio Pelegrin Jaime
2018-07-25
*/

#define Fast512BytesCircularBuffer_SIZE 512 //MUST BE POWER OF 2
#define Fast512BytesCircularBuffer_MASK (Fast512BytesCircularBuffer_SIZE-1)

class Fast512BytesCircularBuffer {
public:
	Fast512BytesCircularBuffer();
	~Fast512BytesCircularBuffer();

	void init(void);

	bool put(unsigned char item);

	bool get(unsigned char* item);

	int fillBuffer(unsigned char* buff, int length);

	void reset(void);

	bool empty(void);

	bool full(void);

	size_t len(void);

	size_t available(void);

private:
	pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;
	unsigned char buf_[Fast512BytesCircularBuffer_SIZE];
	size_t head_ = 0;
	size_t tail_ = 0;
};
