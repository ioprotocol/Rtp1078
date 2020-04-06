//
// Created by 84515 on 2020/4/4.
//
#include "rtmp.h"

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

uint32_t read_uint32(const char* p)
{
	uint32_t v = 0;
	v = v | (*p);
	v = v << 8;
	v = v | (*(p + 1));
	v = v << 8;
	v = v | (*(p + 2));
	v = v << 8;
	v = v | (*(p + 3));
	return v;
}
