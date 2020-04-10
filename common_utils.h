//
// Created by 84515 on 2020/4/4.
//

#ifndef RTP1078_COMMON_UTILS_H
#define RTP1078_COMMON_UTILS_H

#include <stdint.h>
#include <string>

#include "stream_buffer.h"

#define BYTE_ORDER_SWAP16(A)        ((uint16_t)(((A & 0xff00) >> 8) | ((A & 0x00ff) << 8)))

#define BYTE_ORDER_SWAP32(A)        ((uint32_t)(((A & 0xff000000) >> 24) | \
                                                       ((A & 0x00ff0000) >> 8) | \
                                                       ((A & 0x0000ff00) << 8) | \
                                                       ((A & 0x000000ff) << 24)))

#define BYTE_ORDER_SWAP64(A)        ((uint64_t)((      (A & 0xff00000000000000) >> 56)   | \
                                                       ((A & 0x00ff000000000000) >> 40)  | \
                                                       ((A & 0x0000ff0000000000) >> 24)  | \
                                                       ((A & 0x000000ff00000000) >> 8)   | \
                                                       ((A & 0x00000000ff000000) << 8)   | \
                                                       ((A & 0x0000000000ff0000) << 24)  | \
                                                       ((A & 0x000000000000ff00) << 40)  | \
                                                       ((A & 0x00000000000000ff) << 56)))

char integer_to_hex(unsigned char v);

void print_packet(const char* p, uint32_t s);

uint32_t rtmp_message_type(const char* p, uint32_t size);

/**
 * Basic Header and Message Header size
 *
 * @param v
 * @return
 */
uint32_t rtmp_header_size(uint8_t v);

/**
 * Message length field position.
 *
 * @param v
 * @return
 */
uint32_t rtmp_length_pos(uint8_t v);

uint32_t read_uint16(const char* p);

uint32_t read_uint24(const char* p);

uint32_t read_uint32(const char* p);

uint64_t read_uint64(const char* p);

uint32_t search_amf_tree(const char* tree, size_t tree_size, const char* key);

bool h264_is_sps(const char* frame, int size);

uint32_t h264_sps_demux(const char* frame, int nb_frame, std::string& sps);

bool h264_is_pps(const char* frame, int nb_frame);

uint32_t h264_pps_demux(const char* frame, int nb_frame, std::string& pps);

uint32_t h264_mux_sequence_header(std::string& sps, std::string& pps, uint32_t dts, uint32_t pts, std::string& sh);

uint32_t h264_mux_ipb_frame(const char* frame, int nb_frame, std::string& ibp);

uint32_t h264_mux_avc2flv(std::string video, int8_t frame_type, int8_t avc_packet_type, uint32_t dts, uint32_t pts, char** flv, int* nb_flv);

bool srs_avc_startswith_annexb(stream_buffer* stream, int* pnb_start_code);

bool srs_h264_startswith_annexb(char* h264_raw_data, int h264_raw_size, int* pnb_start_code);

int read_h264_frame(char* data, int size, char** pp, int* pnb_start_code, int fps,
		char** frame, int* frame_size, int* dts, int* pts);

int annexb_demux(stream_buffer* stream, char** pframe, int* pnb_frame);

#endif //RTP1078_COMMON_UTILS_H
