//
// Created by 84515 on 2020/4/4.
//
#include <boost/log/trivial.hpp>
#include <cstring>

#include "rtmp.h"

char hex_table[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8',
        '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

char integer_to_hex(unsigned char v)
{
    return hex_table[v];
}

void print_packet(const char* p, uint32_t s)
{
    uint32_t i = 0, j = 0;
    char log[s * 2 + 1];
    while (i < s)
    {
        unsigned char c = *(p + i);
        log[j++] = integer_to_hex(c >> 4);
        log[j++] = integer_to_hex(c & 0xF);
        i++;
    }
    log[j] = '\0';
    BOOST_LOG_TRIVIAL(info) << "size:" << s << ":" << log;
}

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

bool h264_is_sps(const char* frame, int nb_frame)
{
	// 5bits, 7.3.1 NAL unit syntax,
	// ISO_IEC_14496-10-AVC-2003.pdf, page 44.
	//  7: SPS, 8: PPS, 5: I Frame, 1: P Frame
	uint8_t nal_unit_type = (char)frame[0] & 0x1f;
	return nal_unit_type == 7;
}

uint32_t h264_sps_demux(const char* frame, int nb_frame, std::string& sps)
{
	// atleast 1bytes for SPS to decode the type, profile, constrain and level.
	if (nb_frame < 4) {
		return 1;
	}
	sps = std::string(frame, nb_frame);
	return 0;
}

bool h264_is_pps(const char* frame, int nb_frame)
{
	// 5bits, 7.3.1 NAL unit syntax,
	// ISO_IEC_14496-10-AVC-2003.pdf, page 44.
	//  7: SPS, 8: PPS, 5: I Frame, 1: P Frame
	uint8_t nal_unit_type = (char)frame[0] & 0x1f;
	return nal_unit_type == 8;
}

uint32_t h264_pps_demux(const char* frame, int nb_frame, std::string& pps)
{
	if (nb_frame <= 0) {
		return 1;
	}

	pps = std::string(frame, nb_frame);
	return 0;
}

uint32_t h264_mux_avc2flv(std::string video, int8_t frame_type, int8_t avc_packet_type, uint32_t dts, uint32_t pts, char** flv, int* nb_flv)
{
	// for h264 in RTMP video payload, there is 5bytes header:
	//      1bytes, FrameType | CodecID
	//      1bytes, AVCPacketType
	//      3bytes, CompositionTime, the cts.
	// @see: E.4.3 Video Tags, video_file_format_spec_v10_1.pdf, page 78
	int size = (int)video.length() + 5;
	char* data = new char[size];
	char* p = data;

	// @see: E.4.3 Video Tags, video_file_format_spec_v10_1.pdf, page 78
	// Frame Type, Type of video frame.
	// CodecID, Codec Identifier.
	// set the rtmp header 7 = SrsVideoCodecIdAVC
	*p++ = (frame_type << 4) | 7;

	// AVCPacketType
	*p++ = avc_packet_type;

	// CompositionTime
	// pts = dts + cts, or
	// cts = pts - dts.
	// where cts is the header in rtmp video packet payload header.
	uint32_t cts = pts - dts;
	char* pp = (char*)&cts;
	*p++ = pp[2];
	*p++ = pp[1];
	*p++ = pp[0];

	// h.264 raw data.
	memcpy(p, video.data(), video.length());

	*flv = data;
	*nb_flv = size;

	return 0;
}