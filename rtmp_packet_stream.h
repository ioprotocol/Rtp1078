//
// Created by xushuyang on 2020-4-6.
//

#ifndef RTP1078_RTMP_PACKET_STREAM_H
#define RTP1078_RTMP_PACKET_STREAM_H

#include "rtmp.h"

class rtmp_packet_stream
{
 public:
    rtmp_packet_stream(uint32_t chunkSize);

    inline void reset()
    {
        read_index_ = 0;
        write_index_ = 0;
    }

    inline size_t size()
    {
        return write_index_ - read_index_;
    }

    inline char* data()
    {
        return data_;
    }

    inline void write(uint8_t v)
    {
        data_[write_index_++] = v;
    }

	inline void write(char v)
	{
		data_[write_index_++] = v;
	}

	inline void write(uint16_t v)
    {
        data_[write_index_++] = v >> 8;
        data_[write_index_++] = v & 0xFF;
    }

    inline void write_uint24(uint32_t v)
    {
        data_[write_index_++] = (v >> 16) & 0xFF;
        data_[write_index_++] = (v >> 8) & 0xFF;
        data_[write_index_++] = v & 0xFF;
    }

    inline void write(uint32_t v)
    {
        data_[write_index_++] = (v >> 24) & 0xFF;
        data_[write_index_++] = (v >> 16) & 0xFF;
        data_[write_index_++] = (v >> 8) & 0xFF;
        data_[write_index_++] = v & 0xFF;
    }

    inline void write(uint64_t v)
    {
        data_[write_index_++] = (v >> 56) & 0xFF;
        data_[write_index_++] = (v >> 48) & 0xFF;
        data_[write_index_++] = (v >> 40) & 0xFF;
        data_[write_index_++] = (v >> 32) & 0xFF;
        data_[write_index_++] = (v >> 24) & 0xFF;
        data_[write_index_++] = (v >> 16) & 0xFF;
        data_[write_index_++] = (v >> 8) & 0xFF;
        data_[write_index_++] = v & 0xFF;
    }

    inline void write(std::string v)
    {
        write((uint16_t)v.size());
        for (char c : v)
        {
            write((uint8_t)c);
        }
    }

    inline void write_amf_string(std::string v)
    {
        if (v.size() < 1)
        {
            write((uint8_t)NGX_RTMP_AMF_NULL);
            return;
        }
        write((uint8_t)NGX_RTMP_AMF_STRING);
        write((uint16_t)v.size());
        for (char c : v)
        {
            write((uint8_t)c);
        }
    }

    inline void write_amf_string(std::string key, std::string v)
    {
        write(key);
        write_amf_string(v);
    }

    inline void write_amf_number(uint64_t v)
    {
        write((uint8_t)NGX_RTMP_AMF_NUMBER);
        write(BYTE_ORDER_SWAP64(v));
    }

    inline void write_amf_number(std::string key, uint64_t v)
    {
        write(key);
        write_amf_number(v);
    }

    inline void write_amf_boolean(std::string key, uint8_t v)
    {
        write(key);
        write((uint8_t)NGX_RTMP_AMF_BOOLEAN);
        write(v);
    }

    inline void write_amf_object()
    {
        write((uint8_t)NGX_RTMP_AMF_OBJECT);
    }

    inline void write_amf_end()
    {
        write_uint24(NGX_RTMP_AMF_END);
    }

    void create_c0c1_packet();

    void create_c2_packet(const char* p, uint32_t size);

    void create_connect_packet(rtmp_context_t& ctx);

	void create_acknowledgement(uint32_t received_size);

	void create_acknowledgement_window_size(uint32_t window_size);

	void create_create_stream();

	void create_fc_publish_packet(std::string name);

	void create_publish_packet(std::string app, std::string name);

	void create_video_packet(uint8_t fm, uint32_t cs_id, uint32_t delta, uint8_t frame_type, const char* data, size_t size);

	inline void set_chunk_size(uint32_t size)
    {
        chunk_size_ = size;
    }

    void packet_to_chunk();

    void print_debug_info(std::string key);
 private:
    char data_[RTMP_MAX_PACKET_SIZE];
    // rtmp chunk size, default is 128
    uint32_t chunk_size_;
    uint32_t read_index_;
    uint32_t write_index_;
    // debug use
    char debug_data_[RTMP_MAX_PACKET_SIZE * 2];
};

#endif //RTP1078_RTMP_PACKET_STREAM_H
