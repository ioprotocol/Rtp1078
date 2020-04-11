//
// Created by 84515 on 2020/4/11.
//
#include <boost/log/trivial.hpp>

#include "h264_stream.h"

h264_stream::h264_stream()
{
	reader_ = buf_;
	writer_ = buf_;
}

int h264_stream::size()
{
	return writer_ - reader_;
}

int h264_stream::availabe()
{
	return reader_ + MAX_SIZE - writer_;
}

void h264_stream::reset()
{
	reader_ = buf_;
	writer_ = buf_;
}

char* h264_stream::data()
{
	return reader_;
}

void h264_stream::append(char* data, size_t size)
{
	if (availabe() < size)
	{
		BOOST_LOG_TRIVIAL(error) << "h264 stream buffer is overfull \n";
		return;
	}

	if (reader_ - buf_ > MAX_SIZE / 2)
	{
		std::memcpy(buf_, reader_, writer_ - reader_);
		writer_ = writer_ - (reader_ - buf_);
		reader_ = buf_;
		BOOST_LOG_TRIVIAL(info) << "h264 stream buffer adjust position\n";
	}

	std::memcpy(writer_, data, size);
	writer_ += size;
}

bool h264_stream::find_nalu(char** nalu, size_t* size)
{
	if (this->size() < 4)
	{
		return false;
	}

	char* p = reader_;
	// NALU begin with 0x00 00 00 01
	// make sure reader_ is the beginning of the NALU
	while (p < writer_)
	{
		if (p + 4 < writer_)
		{
			if (*p == 0 && *(p + 1) == 0 && *(p + 2) == 0 && *(p + 3) == 1)
			{
				reader_ = p;
				break;
			}
		}
		p++;
	}
	if (p == writer_)
	{
		reset();
		return false;
	}

	p = reader_ + 4;
	// NALU begin with 0x00 00 00 01
	// make sure reader_ is the beginning of the NALU
	while (p < writer_)
	{
		if (p + 4 < writer_)
		{
			if (*p == 0 && *(p + 1) == 0 && *(p + 2) == 0 && *(p + 3) == 1)
			{
				break;
			}
		}
		p++;
	}
	if (p == writer_)
	{
		return false;
	}

	*nalu = reader_;
	*size = p - reader_;
	return true;
}

void h264_stream::skip(size_t size)
{
	this->reader_ += size;
}
