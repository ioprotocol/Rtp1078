//
// Created by xushuyang on 2020-4-9.
//

#include <cstring>

#include "stream_buffer.h"

stream_buffer::stream_buffer()
{
	p = bytes = NULL;
	nb_bytes = 0;
}

stream_buffer::stream_buffer(char* b, int nb_b)
{
	p = bytes = b;
	nb_bytes = nb_b;
}

stream_buffer::~stream_buffer()
{
}

char* stream_buffer::data()
{
	return bytes;
}

int stream_buffer::size()
{
	return nb_bytes;
}

int stream_buffer::pos()
{
	return (int)(p - bytes);
}

int stream_buffer::left()
{
	return nb_bytes - (int)(p - bytes);
}

bool stream_buffer::empty()
{
	return !bytes || (p >= bytes + nb_bytes);
}

bool stream_buffer::require(int required_size)
{
	return required_size <= nb_bytes - (p - bytes);
}

void stream_buffer::skip(int size)
{
	p += size;
}

int8_t stream_buffer::read_1bytes()
{
	return (int8_t)*p++;
}

int16_t stream_buffer::read_2bytes()
{
	int16_t value;
	char* pp = (char*)&value;
	pp[1] = *p++;
	pp[0] = *p++;

	return value;
}

int32_t stream_buffer::read_3bytes()
{
	int32_t value = 0x00;
	char* pp = (char*)&value;
	pp[2] = *p++;
	pp[1] = *p++;
	pp[0] = *p++;

	return value;
}

int32_t stream_buffer::read_4bytes()
{
	int32_t value;
	char* pp = (char*)&value;
	pp[3] = *p++;
	pp[2] = *p++;
	pp[1] = *p++;
	pp[0] = *p++;

	return value;
}

int64_t stream_buffer::read_8bytes()
{
	int64_t value;
	char* pp = (char*)&value;
	pp[7] = *p++;
	pp[6] = *p++;
	pp[5] = *p++;
	pp[4] = *p++;
	pp[3] = *p++;
	pp[2] = *p++;
	pp[1] = *p++;
	pp[0] = *p++;

	return value;
}

std::string stream_buffer::read_string(int len)
{
	std::string value;
	value.append(p, len);

	p += len;

	return value;
}

void stream_buffer::read_bytes(char* data, int size)
{
	std::memcpy(data, p, size);

	p += size;
}

void stream_buffer::write_1bytes(int8_t value)
{
	*p++ = value;
}

void stream_buffer::write_2bytes(int16_t value)
{
	char* pp = (char*)&value;
	*p++ = pp[1];
	*p++ = pp[0];
}

void stream_buffer::write_4bytes(int32_t value)
{
	char* pp = (char*)&value;
	*p++ = pp[3];
	*p++ = pp[2];
	*p++ = pp[1];
	*p++ = pp[0];
}

void stream_buffer::write_3bytes(int32_t value)
{
	char* pp = (char*)&value;
	*p++ = pp[2];
	*p++ = pp[1];
	*p++ = pp[0];
}

void stream_buffer::write_8bytes(int64_t value)
{
	char* pp = (char*)&value;
	*p++ = pp[7];
	*p++ = pp[6];
	*p++ = pp[5];
	*p++ = pp[4];
	*p++ = pp[3];
	*p++ = pp[2];
	*p++ = pp[1];
	*p++ = pp[0];
}

void stream_buffer::write_string(std::string value)
{
	memcpy(p, value.data(), value.length());
	p += value.length();
}

void stream_buffer::write_bytes(const char* data, int size)
{
	std::memcpy(p, data, size);
	p += size;
}