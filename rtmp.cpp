//
// Created by 84515 on 2020/4/4.
//
#include "rtmp.h"

uint32_t get_rtmp_message_type(const char *p, uint32_t size)
{
    uint8_t fm = ((*p) >> 6) & 0x3;
    uint8_t stream_id = (*p) & 0x3F;

    uint32_t chunk_packet_size = 1;
    // Chunk header length
    if (stream_id == 0) {
        chunk_packet_size = 2;
    }
    if (stream_id == 1) {
        chunk_packet_size = 3;
    }

    if (fm == 0 || fm == 1) {
        return *(p + chunk_packet_size + 6);
    }

    return NGX_RTMP_MSG_UNDEFINED;
}

void write_uint16(char **p, uint16_t value) {
    **p = value >> 8;
    *p = *p + 1;
    **p = value & 0xFF;
    *p = *p + 1;
}

void write_uint24(char **p, uint32_t value) {
    **p = (value >> 16) & 0xFF;
    *p = *p + 1;
    **p = (value >> 8) & 0xFF;
    *p = *p + 1;
    **p = value & 0xFF;
    *p = *p + 1;
}

void write_uint32(char **p, uint32_t value) {
    **p = (value >> 24) & 0xFF;
    *p = *p + 1;
    **p = (value >> 16) & 0xFF;
    *p = *p + 1;
    **p = (value >> 8) & 0xFF;
    *p = *p + 1;
    **p = value & 0xFF;
    *p = *p + 1;
}

void write_uint64(char **p, uint64_t value) {
    write_uint32(p, value >> 32);
    write_uint32(p, value & 0xFFFFFFFF);
}

void write_string(char **p, std::string value) {
    write_uint16(p, value.size());
    for (char v : value) {
        **p = v;
        *p = *p + 1;
    }
}
