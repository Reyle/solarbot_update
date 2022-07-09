//#pragma once
#include <cstdio>
#include "Fast512BytesCircularBuffer.h"


/*
Fast 512 bytes circular buffer
Eng. Deulis Antonio Pelegrin Jaime
2018-07-25
*/


Fast512BytesCircularBuffer::Fast512BytesCircularBuffer()
{
	init();
}

Fast512BytesCircularBuffer :: ~Fast512BytesCircularBuffer()
{

}

void Fast512BytesCircularBuffer::init(void)
{
	head_ = 0;
	tail_ = 0;
}

bool Fast512BytesCircularBuffer::put(unsigned char item)
{
	if (full())
		return false;
	
	pthread_mutex_lock(&mutex_);
	buf_[(head_++) & Fast512BytesCircularBuffer_MASK] = item;
	pthread_mutex_unlock(&mutex_);

	return true;
}

bool Fast512BytesCircularBuffer::get(unsigned char* item)
{
	if (empty())
		return false;

	pthread_mutex_lock(&mutex_);
	*item = buf_[(tail_++) & Fast512BytesCircularBuffer_MASK];
	pthread_mutex_unlock(&mutex_);

	return true;
}

int Fast512BytesCircularBuffer::fillBuffer(unsigned char * buff, int length)
{
	int c = len();
	if (c > 0)
	{
		pthread_mutex_lock(&mutex_);
		if (c > length)
			c = length;

		for (int i = 0; i < c; i++)
		{
			buff[i] = buf_[(tail_++) & Fast512BytesCircularBuffer_MASK];
		}

		pthread_mutex_unlock(&mutex_);
	}
	return c;
}

void Fast512BytesCircularBuffer::reset(void)
{
	pthread_mutex_lock(&mutex_);
	head_ = tail_;
	pthread_mutex_unlock(&mutex_);
}

bool Fast512BytesCircularBuffer::empty(void)
{
	//if head and tail are equal, we are empty
	return head_ == tail_;
}

bool Fast512BytesCircularBuffer::full(void)
{
	//If tail is ahead the head by 1, we are full
	return ((head_ + 1) & Fast512BytesCircularBuffer_MASK) == (tail_ & Fast512BytesCircularBuffer_MASK);
}

size_t Fast512BytesCircularBuffer::len(void)
{
	return head_ - tail_;
}

size_t Fast512BytesCircularBuffer::available(void)
{
	return  Fast512BytesCircularBuffer_SIZE - (head_ - tail_);
}


