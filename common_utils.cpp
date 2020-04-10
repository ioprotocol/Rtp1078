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
			key_value.append(std::to_string((uint8_t)*p)).append(";");
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
	if (nb_frame < 4)
	{
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
	if (nb_frame <= 0)
	{
		return 1;
	}

	pps = std::string(frame, nb_frame);
	return 0;
}

uint32_t h264_mux_sequence_header(std::string& sps, std::string& pps, uint32_t dts, uint32_t pts, std::string& sh)
{
	// 5bytes sps/pps header:
	//      configurationVersion, AVCProfileIndication, profile_compatibility,
	//      AVCLevelIndication, lengthSizeMinusOne
	// 3bytes size of sps:
	//      numOfSequenceParameterSets, sequenceParameterSetLength(2B)
	// Nbytes of sps.
	//      sequenceParameterSetNALUnit
	// 3bytes size of pps:
	//      numOfPictureParameterSets, pictureParameterSetLength
	// Nbytes of pps:
	//      pictureParameterSetNALUnit
	int nb_packet = 5 + (3 + (int)sps.length()) + (3 + (int)pps.length());
	char* packet = new char[nb_packet];

	stream_buffer stream(packet, nb_packet);
	// decode the SPS:
	// @see: 7.3.2.1.1, ISO_IEC_14496-10-AVC-2012.pdf, page 62
	if (true)
	{
//		srs_assert((int)sps.length() >= 4);
		char* frame = (char*)sps.data();

		// @see: Annex A Profiles and levels, ISO_IEC_14496-10-AVC-2003.pdf, page 205
		//      Baseline profile profile_idc is 66(0x42).
		//      Main profile profile_idc is 77(0x4d).
		//      Extended profile profile_idc is 88(0x58).
		uint8_t profile_idc = frame[1];
		//uint8_t constraint_set = frame[2];
		uint8_t level_idc = frame[3];

		// generate the sps/pps header
		// 5.3.4.2.1 Syntax, ISO_IEC_14496-15-AVC-format-2012.pdf, page 16
		// configurationVersion
		stream.write_1bytes(0x01);
		// AVCProfileIndication
		stream.write_1bytes(profile_idc);
		// profile_compatibility
		stream.write_1bytes(0x00);
		// AVCLevelIndication
		stream.write_1bytes(level_idc);
		// lengthSizeMinusOne, or NAL_unit_length, always use 4bytes size,
		// so we always set it to 0x03.
		stream.write_1bytes(0x03);
	}

	// sps
	if (true)
	{
		// 5.3.4.2.1 Syntax, ISO_IEC_14496-15-AVC-format-2012.pdf, page 16
		// numOfSequenceParameterSets, always 1
		stream.write_1bytes(0x01);
		// sequenceParameterSetLength
		stream.write_2bytes((int16_t)sps.length());
		// sequenceParameterSetNALUnit
		stream.write_string(sps);
	}

	// pps
	if (true)
	{
		// 5.3.4.2.1 Syntax, ISO_IEC_14496-15-AVC-format-2012.pdf, page 16
		// numOfPictureParameterSets, always 1
		stream.write_1bytes(0x01);
		// pictureParameterSetLength
		stream.write_2bytes((int16_t)pps.length());
		// pictureParameterSetNALUnit
		stream.write_string(pps);
	}

	// TODO: FIXME: for more profile.
	// 5.3.4.2.1 Syntax, ISO_IEC_14496-15-AVC-format-2012.pdf, page 16
	// profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 144
	sh.append(packet, nb_packet);

	delete[] packet;
	return 0;
}

uint32_t h264_mux_ipb_frame(const char* frame, int nb_frame, std::string& ibp)
{
	// 4bytes size of nalu:
	//      NALUnitLength
	// Nbytes of nalu.
	//      NALUnit
	int nb_packet = 4 + nb_frame;
	char* packet = new char[nb_packet];

	// use stream to generate the h264 packet.
	stream_buffer stream(packet, nb_packet);
	// 5.3.4.2.1 Syntax, ISO_IEC_14496-15-AVC-format-2012.pdf, page 16
	// lengthSizeMinusOne, or NAL_unit_length, always use 4bytes size
	uint32_t NAL_unit_length = nb_frame;

	// mux the avc NALU in "ISO Base Media File Format"
	// from ISO_IEC_14496-15-AVC-format-2012.pdf, page 20
	// NALUnitLength
	stream.write_4bytes(NAL_unit_length);
	// NALUnit
	stream.write_bytes(frame, nb_frame);

//	ibp = std::string(packet, nb_packet);
	ibp.append(packet, nb_packet);

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
	std::memcpy(p, video.data(), video.length());

	*flv = data;
	*nb_flv = size;

	return 0;
}

bool srs_avc_startswith_annexb(stream_buffer* stream, int* pnb_start_code)
{
	if (!stream) {
		return false;
	}

	char* bytes = stream->data() + stream->pos();
	char* p = bytes;

	for (;;) {
		if (!stream->require((int)(p - bytes + 3))) {
			return false;
		}

		// not match
		if (p[0] != (char)0x00 || p[1] != (char)0x00) {
			return false;
		}

		// match N[00] 00 00 01, where N>=0
		if (p[2] == (char)0x01) {
			if (pnb_start_code) {
				*pnb_start_code = (int)(p - bytes) + 3;
			}
			return true;
		}

		p++;
	}

	return false;
}

bool srs_h264_startswith_annexb(char* h264_raw_data, int h264_raw_size, int* pnb_start_code)
{
	stream_buffer stream(h264_raw_data, h264_raw_size);
	return srs_avc_startswith_annexb(&stream, pnb_start_code);
}

int read_h264_frame(char* data, int size, char** pp, int* pnb_start_code, int fps,
		char** frame, int* frame_size, int* dts, int* pts)
{
	char* p = *pp;

	// @remark, for this demo, to publish h264 raw file to SRS,
	// we search the h264 frame from the buffer which cached the h264 data.
	// please get h264 raw data from device, it always a encoded frame.
	if (!srs_h264_startswith_annexb(p, size - (p - data), pnb_start_code))
	{
		return -1;
	}

	// @see srs_write_h264_raw_frames
	// each frame prefixed h.264 annexb header, by N[00] 00 00 01, where N>=0,
	// for instance, frame = header(00 00 00 01) + payload(67 42 80 29 95 A0 14 01 6E 40)
	*frame = p;
	p += *pnb_start_code;

	for (; p < data + size; p++)
	{
		if (srs_h264_startswith_annexb(p, size - (p - data), NULL))
		{
			break;
		}
	}

	*pp = p;
	*frame_size = p - *frame;
	if (*frame_size <= 0)
	{
		return -1;
	}

	// @remark, please get the dts and pts from device,
	// we assume there is no B frame, and the fps can guess the fps and dts,
	// while the dts and pts must read from encode lib or device.
	*dts += 1000 / fps;
	*pts = *dts;

	return 0;
}

int annexb_demux(stream_buffer* stream, char** pframe, int* pnb_frame)
{
	*pframe = NULL;
	*pnb_frame = 0;

	while (!stream->empty()) {
		// each frame must prefixed by annexb format.
		// about annexb, @see ISO_IEC_14496-10-AVC-2003.pdf, page 211.
		int pnb_start_code = 0;
		if (!srs_avc_startswith_annexb(stream, &pnb_start_code)) {
			return -1;
		}
		int start = stream->pos() + pnb_start_code;

		// find the last frame prefixed by annexb format.
		stream->skip(pnb_start_code);
		while (!stream->empty()) {
			if (srs_avc_startswith_annexb(stream, NULL)) {
				break;
			}
			stream->skip(1);
		}

		// demux the frame.
		*pnb_frame = stream->pos() - start;
		*pframe = stream->data() + start;
		break;
	}

	return 0;
}