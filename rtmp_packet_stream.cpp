//
// Created by xushuyang on 2020-4-6.
//
#include <boost/log/trivial.hpp>
#include <lzma.h>
#include "rtmp_packet_stream.h"

rtmp_packet_stream::rtmp_packet_stream(uint32_t chunkSize) : chunk_size_(chunkSize), read_index_(0), write_index_(0)
{
}

rtmp_packet_stream& operator<<(rtmp_packet_stream& stream, uint8_t v)
{
	stream.write(v);
	return stream;
}

rtmp_packet_stream& operator<<(rtmp_packet_stream& stream, uint16_t v)
{
	stream.write(v);
	return stream;
}

rtmp_packet_stream& operator<<(rtmp_packet_stream& stream, uint32_t v)
{
	stream.write(v);
	return stream;
}

rtmp_packet_stream& operator<<(rtmp_packet_stream& stream, uint64_t v)
{
	stream.write(v);
	return stream;
}

rtmp_packet_stream& operator<<(rtmp_packet_stream& stream, std::string v)
{
	stream.write(v);
	return stream;
}

void rtmp_packet_stream::packet_to_chunk()
{
	uint32_t head_size = rtmp_header_size((uint8_t)data_[0]);
	uint32_t length_field_pos = rtmp_length_pos((uint8_t)data_[0]);
	uint32_t payload_size = this->size() - head_size;
	// update payload size field
	data_[length_field_pos] = (char)((payload_size >> 16) & 0xFF);
	data_[length_field_pos + 1] = (char)((payload_size >> 8) & 0xFF);
	data_[length_field_pos + 2] = (char)(payload_size & 0xFF);
	// split 3 chunk
	char* m = &data_[head_size];
	char* n = &data_[this->size()];
	while (n - m > 128)
	{
		m = m + 128;
		n = n + 1;
		while (n > m)
		{
			*n = *(n - 1);
			n--;
		}
		*m = (3 << 6) | 3;
		m++;
		write_index_++;
		n = &data_[this->size()];;
	}
}

void rtmp_packet_stream::print_debug_info()
{
	uint32_t i = 0, j = 0, s = this->size();
	while (i < s)
	{
		unsigned char c = data_[i];
		debug_data_[j++] = integer_to_hex(c >> 4);
		debug_data_[j++] = integer_to_hex(c & 0xF);
		i++;
	}
	debug_data_[j] = '\0';
	BOOST_LOG_TRIVIAL(info) << "length:" << s << ":" << debug_data_;
}

void rtmp_packet_stream::create_c0c1_packet()
{
	reset();
	write((uint8_t)NGX_RTMP_VERSION);
	write((uint32_t)0);
	write((uint32_t)0);
	for (size_t i = 0; i < RTMP_C1_LENGTH - 8; i++)
	{
		write((uint8_t)((i * 111) & 0xFF));
	}
}

void rtmp_packet_stream::create_c2_packet(const char* p, uint32_t size)
{
	reset();
	for (int i = 0; i < size; i++)
	{
		write((uint8_t)(*(p + i)));
	}
}

void rtmp_packet_stream::create_acknowledgement(uint32_t received_size)
{
	reset();
	write((uint8_t)2);
	// timestamp
	write_uint24(0);
	// payload_size
	write_uint24(0);
	// message type id
	write((uint8_t)NGX_RTMP_MSG_ACK);
	// message stream id
	write((uint32_t)0);
	write((uint32_t)received_size);
	packet_to_chunk();
}

void rtmp_packet_stream::create_acknowledgement_window_size(uint32_t window_size)
{
	reset();
	write((uint8_t)2);
	// timestamp
	write_uint24(0);
	// payload_size
	write_uint24(0);
	// message type id
	write((uint8_t)NGX_RTMP_MSG_ACK_SIZE);
	// message stream id
	write((uint32_t)0);
	write((uint32_t)window_size);
	write((uint8_t)2);
	packet_to_chunk();
}

void rtmp_packet_stream::create_connect_packet(rtmp_context_t& ctx)
{
	reset();
	write((uint8_t)3);
	// timestamp
	write_uint24(0);
	// payload_size
	write_uint24(0);
	// message type id
	write((uint8_t)NGX_RTMP_MSG_AMF_CMD);
	// message stream id
	write((uint32_t)0);
	// payload
	write_amf_string("connect");
	write_amf_number(ctx.transaction_id);
	write_amf_object();
	write_amf_number("objectEncoding", 0);
	write_amf_boolean("fpad", ctx.fpad);
	write_amf_string("flashVer", ctx.flashver);
	write_amf_number("capabilities", ctx.capabilities);
	write_amf_number("audioCodecs", ctx.audio_codecs);
	write_amf_number("videoCodecs", ctx.vidio_codecs);
	write_amf_number("videoFunction", ctx.vidio_function);
	write_amf_string("swfUrl", ctx.swf_url);
	write_amf_string("pageUrl", ctx.page_url);
	write_amf_string("app", ctx.app);
	write_amf_string("tcUrl", ctx.tc_url);
	write_amf_end();

	packet_to_chunk();
}


void rtmp_packet_stream::create_create_stream()
{
	reset();
	write((uint8_t)3);
	// timestamp
	write_uint24(0);
	// payload_size
	write_uint24(0);
	// message type id
	write((uint8_t)NGX_RTMP_MSG_AMF_CMD);
	// message stream id
	write((uint32_t)0);
	// payload
	write_amf_string("createStream");
	write_amf_number(2);
	write((uint8_t)NGX_RTMP_AMF_NULL);
	packet_to_chunk();
}

void rtmp_packet_stream::create_fc_publish_packet(std::string name)
{
	reset();
	write((uint8_t)3);
	// timestamp
	write_uint24(0);
	// payload_size
	write_uint24(0);
	// message type id
	write((uint8_t)NGX_RTMP_MSG_AMF_CMD);
	// message stream id
	write((uint32_t)0);
	// payload
	write_amf_string("FcPublish");
	write_amf_number(3);
	write((uint8_t)NGX_RTMP_AMF_NULL);
	write_amf_string(name);

	packet_to_chunk();
}

void rtmp_packet_stream::create_publish_packet(std::string app, std::string name)
{
	reset();
	write((uint8_t)4);
	// timestamp
	write_uint24(0);
	// payload_size
	write_uint24(0);
	// message type id
	write((uint8_t)NGX_RTMP_MSG_AMF_CMD);
	// message stream id
	write((uint32_t)1);
	// payload
	write_amf_string("publish");
	write((uint8_t)NGX_RTMP_AMF_NULL);
	write_amf_string(name);
	write_amf_string(app);

	packet_to_chunk();
}

void rtmp_packet_stream::create_video_packet(uint8_t fm, uint32_t cs_id, uint32_t delta, uint8_t frame_type, const char* data, size_t size)
{
	reset();
	if (frame_type == 3)
	{
		write((uint8_t)((fm << 6) | NGX_RTMP_MSG_AUDIO));
	}
	else
	{
		write((uint8_t)((fm << 6) | NGX_RTMP_MSG_VIDEO));
	}
	if (fm < 2)
	{
		// timestamp
		write_uint24(delta);
		// payload_size
		write_uint24(0);
		if (frame_type == 3)
		{
			write((uint8_t)((fm << 6) | NGX_RTMP_MSG_AUDIO));
		}
		else
		{
			// message type id
			write((uint8_t)NGX_RTMP_MSG_VIDEO);
		}
	}
	if (fm == 0)
	{
		// message stream id
		write((uint32_t)(BYTE_ORDER_SWAP32(cs_id)));
	}
	if (frame_type == 0)
	{
		// I Frame -> RTMP keyframe(0x17)
		write((uint8_t)0x17);
	}
	else if (frame_type == 1)
	{
		// P Frame -> RTMP inter-frame(0x27)
		write((uint8_t)0x27);
	}
	for (uint32_t i = 0; i < size; i++)
	{
		write((uint8_t)(*(data + i)));
	}
	packet_to_chunk();
}



