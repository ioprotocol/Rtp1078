//
// Created by 84515 on 2020/4/4.
//
#include <cstring>
#include <boost/log/trivial.hpp>
#include <stdlib.h>

#include "rtmp.h"
#include "common_utils.h"

uint32_t rtmp_message_type(const char* p, uint32_t size)
{
	uint8_t fm = ((*p) >> 6) & 0x3;
	uint8_t stream_id = (*p) & 0x3F;

	uint32_t chunk_packet_size = 1;
	// Chunk header length
	if (stream_id == 0)
	{
		chunk_packet_size = 2;
	}
	if (stream_id == 1)
	{
		chunk_packet_size = 3;
	}

	if (fm == 0 || fm == 1)
	{
		return *(p + chunk_packet_size + 6);
	}

	return NGX_RTMP_MSG_UNDEFINED;
}

uint32_t rtmp_header_size(uint8_t v)
{
	uint8_t fm = (uint8_t)((v >> 6) & 0x3);
	uint8_t stream_id = (uint8_t)(v & 0x3F);
	uint32_t length = 1;

	// Chunk header length
	if (stream_id == 0)
	{
		length = 2;
	}
	if (stream_id == 1)
	{
		length = 3;
	}
	// Chunk message header length
	switch (fm)
	{
	case 0:
		length += 11;
		break;
	case 1:
		length += 7;
		break;
	case 2:
		length += 3;
		break;
	case 3:
		break;
	}
	return length;
}

uint32_t rtmp_length_pos(uint8_t v)
{
	uint8_t fm = (uint8_t)((v >> 6) & 0x3);
	uint8_t stream_id = (uint8_t)(v & 0x3F);

	uint32_t length = 1;
	// Chunk header length
	if (stream_id == 0)
	{
		length = 2;
	}
	if (stream_id == 1)
	{
		length = 3;
	}
	// Chunk message header length
	switch (fm)
	{
	case 0:
		length += 3;
		break;
	case 1:
		length += 3;
		break;
	case 2:
		length = -1;
		break;
	case 3:
		length = -1;
		break;
	}
	return length;
}

uint32_t read_uint16(const char* p)
{
	uint32_t v = 0;
	v = v | ((*p) & 0xFF);
	v = v << 8;
	v = v | (*(p + 1) & 0xFF);
	return v;
}

uint32_t read_uint24(const char* p)
{
	uint32_t v = 0;
	v = v | ((*p) & 0xFF);
	v = v << 8;
	v = v | (*(p + 1) & 0xFF);
	v = v << 8;
	v = v | (*(p + 2) & 0xFF);
	return v;
}

uint32_t read_uint32(const char* p)
{
	uint32_t v = 0;
	v = v | ((*p) & 0xFF);
	v = v << 8;
	v = v | (*(p + 1) & 0xFF);
	v = v << 8;
	v = v | (*(p + 2) & 0xFF);
	v = v << 8;
	v = v | (*(p + 3) & 0xFF);
	return v;
}

uint64_t read_uint64(const char* p)
{
	uint64_t h = read_uint32(p);
	uint64_t l = read_uint32(p);
	return (h << 32) | l;
}

uint32_t search_amf_tree(const char* tree, size_t tree_size, const char* key)
{
	uint32_t result = 0;
	uint32_t head_size = rtmp_header_size(*tree);
	uint32_t size;

	const char* p = &tree[head_size];
	uint8_t is_object = 0;

	std::string key_value;

	while (p - tree < tree_size)
	{
		// property name
		if (is_object)
		{
			// check if object is closed
			if (read_uint24(p) == NGX_RTMP_AMF_END)
			{
				p += 3;
				is_object = 0;
			}
			else
			{
				size = read_uint16(p);
				p += 2;
				key_value.append(p, size);
				key_value.append("=");
				p += size;
			}
		}

		uint8_t amf_type = *p++;
		switch (amf_type)
		{
		case NGX_RTMP_AMF_NUMBER:
			key_value.append(std::to_string(read_uint64(p))).append(";");
			p += 8;
			break;
		case NGX_RTMP_AMF_BOOLEAN:
			key_value.append(std::to_string((uint8_t) *p)).append(";");
			p += 1;
			break;
		case NGX_RTMP_AMF_STRING:
			size = read_uint16(p);
			p += 2;
			key_value.append(p, size).append(";");
			if (!std::strncmp(p, key, size))
			{
				result = 1;
			}
			p += size;
			break;
		case NGX_RTMP_AMF_OBJECT:
			is_object = 1;
			break;
		case NGX_RTMP_AMF_NULL:
		case NGX_RTMP_AMF_ARRAY_NULL:
		case NGX_RTMP_AMF_MIXED_ARRAY:
		case NGX_RTMP_AMF_ARRAY:
		case NGX_RTMP_AMF_VARIANT_:
			break;
		}
	}
	BOOST_LOG_TRIVIAL(info) << "RTMP AMF CMD: " << key_value << "\n";
	key_value.clear();

	return result;
}
