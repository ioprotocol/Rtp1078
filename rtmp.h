//
// Created by xushuyang on 2020-4-3.
//

#ifndef RTP1078_RTMP_H
#define RTP1078_RTMP_H

#include "common_utils.h"

typedef struct
{
	std::string app;
	std::string name;

	uint64_t transaction_id;
	std::string flashver;
	std::string swf_url;
	std::string tc_url;
	uint8_t fpad;
	uint64_t audio_codecs;
	uint64_t vidio_codecs;
	uint64_t vidio_function;
	std::string page_url;
	uint64_t capabilities;

	// about SPS, @see: 7.3.2.1.1, ISO_IEC_14496-10-AVC-2012.pdf, page 62
	std::string h264_sps;
	std::string h264_pps;
	// whether the sps and pps sent,
	// @see https://github.com/ossrs/srs/issues/203
	bool h264_sps_pps_sent;
	// only send the ssp and pps when both changed.
	// @see https://github.com/ossrs/srs/issues/204
	bool h264_sps_changed;
	bool h264_pps_changed;
	// the aac sequence header.
	std::string aac_specific_config;
	// user set timeout, in ms.
	int64_t stimeout;
	int64_t rtimeout;
} rtmp_context_t;

// H264 NALU 的类型，在分隔符后的第一个字节低5位

#define NALU_TYPE_SLICE                1
#define NALU_TYPE_DPA                    2
#define NALU_TYPE_DPB                    3
#define NALU_TYPE_DPC                    4
#define NALU_TYPE_IDR                    5    /**关键帧***/
#define NALU_TYPE_SEI                    6    /*****曾强帧******/
#define NALU_TYPE_SPS                    7
#define NALU_TYPE_PPS                    8
#define NALU_TYPE_AUD                    9
#define NALU_TYPE_EOSEQ                10
#define NALU_TYPE_EOSTREAM                11
#define NALU_TYPE_FILL                    12

/**
 * RTMP 视频帧Body第一个字节含义:
 * 前4bits表示类型：
 *·1-- keyframe
 *·2 -- inner frame
 *·3 -- disposable inner frame （h.263 only）
 *·4 -- generated keyframe
 *后4bits表示解码器ID：
 *·2 -- seronson h.263
 *·3 -- screen video
 *·4 -- On2 VP6
 *·5 -- On2 VP6 with alpha channel
 *·6 -- Screen video version 2
 *·7 -- AVC (h.264)
 */
#define RTMP_MAX_PACKET_SIZE            16 *1024

#define NGX_RTMP_VERSION                3

#define NGX_LOG_DEBUG_RTMP              NGX_LOG_DEBUG_CORE

#define NGX_RTMP_DEFAULT_CHUNK_SIZE     128

/* RTMP message types */
#define NGX_RTMP_MSG_CHUNK_SIZE         1
#define NGX_RTMP_MSG_ABORT              2
#define NGX_RTMP_MSG_ACK                3
#define NGX_RTMP_MSG_USER               4
#define NGX_RTMP_MSG_ACK_SIZE           5
#define NGX_RTMP_MSG_BANDWIDTH          6
#define NGX_RTMP_MSG_EDGE               7
#define NGX_RTMP_MSG_AUDIO              8
#define NGX_RTMP_MSG_VIDEO              9
#define NGX_RTMP_MSG_AMF3_META          15
#define NGX_RTMP_MSG_AMF3_SHARED        16
#define NGX_RTMP_MSG_AMF3_CMD           17
#define NGX_RTMP_MSG_AMF_META           18
#define NGX_RTMP_MSG_AMF_SHARED         19
#define NGX_RTMP_MSG_AMF_CMD            20
#define NGX_RTMP_MSG_AGGREGATE          22
#define NGX_RTMP_MSG_MAX                22
#define NGX_RTMP_MSG_UNDEFINED          255

#define NGX_RTMP_MAX_CHUNK_SIZE         10485760
#define NGX_RTMP_MAX_WINDOW_SIZE        500000

#define NGX_RTMP_CONNECT                NGX_RTMP_MSG_MAX + 1
#define NGX_RTMP_DISCONNECT             NGX_RTMP_MSG_MAX + 2
#define NGX_RTMP_HANDSHAKE_DONE         NGX_RTMP_MSG_MAX + 3
#define NGX_RTMP_MAX_EVENT              NGX_RTMP_MSG_MAX + 4


/* RMTP control message types */
#define NGX_RTMP_USER_STREAM_BEGIN      0
#define NGX_RTMP_USER_STREAM_EOF        1
#define NGX_RTMP_USER_STREAM_DRY        2
#define NGX_RTMP_USER_SET_BUFLEN        3
#define NGX_RTMP_USER_RECORDED          4
#define NGX_RTMP_USER_PING_REQUEST      6
#define NGX_RTMP_USER_PING_RESPONSE     7
#define NGX_RTMP_USER_UNKNOWN           8
#define NGX_RTMP_USER_BUFFER_END        31

/* Chunk header:
 *   max 3  basic header
 * + max 11 message header
 * + max 4  extended header (timestamp) */
#define NGX_RTMP_MAX_CHUNK_HEADER       1

/* basic types */
#define NGX_RTMP_AMF_NUMBER             0x00
#define NGX_RTMP_AMF_BOOLEAN            0x01
#define NGX_RTMP_AMF_STRING             0x02
#define NGX_RTMP_AMF_OBJECT             0x03
#define NGX_RTMP_AMF_NULL               0x05
#define NGX_RTMP_AMF_ARRAY_NULL         0x06
#define NGX_RTMP_AMF_MIXED_ARRAY        0x08
#define NGX_RTMP_AMF_END                0x09
#define NGX_RTMP_AMF_ARRAY              0x0a

/* extended types */
#define NGX_RTMP_AMF_INT8               0x0100
#define NGX_RTMP_AMF_INT16              0x0101
#define NGX_RTMP_AMF_INT32              0x0102
#define NGX_RTMP_AMF_VARIANT_           0x0103

/* r/w flags */
#define NGX_RTMP_AMF_OPTIONAL           0x1000
#define NGX_RTMP_AMF_TYPELESS           0x2000
#define NGX_RTMP_AMF_CONTEXT            0x4000

#define NGX_RTMP_AMF_VARIANT            (NGX_RTMP_AMF_VARIANT_\
                                        |NGX_RTMP_AMF_TYPELESS)

#define SUPPORT_VID_H264                0x0080
#define SUPPORT_SND_AAC                 0x0400

#define RTMP_C1_LENGTH                  1536
#define RTMP_S1_LENGTH                  1536
#define RTMP_S2_LENGTH                  1536

// Chunk format
/**
  *      0              1               2                  3
 * +--------------+----------------+------------------+------------+
 * | Basic Header | Message Header | Extend Timestamp | Chunk Data |
 * +--------------+----------------+------------------+------------+
 * |<---------------- Chunk Header ------------------>|
 *
 *
 * fmt = 0  11 bytes
 *      0              1               2                  3
 * +-----------+--------------+----------------+-------------------+
 * |               timestamp                   |   message length  |
 * +-----------+--------------+----------------+-------------------+
 * |   message length (cont)  |message type id |   msg stream id   |
 * +-----------+--------------+----------------+-------------------+
 * |       message stream id (cont)            |
 * +-----------+--------------+----------------+-------------------+
 *
 * fmt = 1  7 bytes
 *      0              1               2                  3
 * +-----------+--------------+----------------+-------------------+
 * |               timestamp                   |   message length  |
 * +-----------+--------------+----------------+-------------------+
 * |   message length (cont)  |message type id |
 * +-----------+--------------+----------------+-------------------+
 *
 * fmt = 2  3 bytes
 *      0              1               2                  3
 * +-----------+--------------+----------------+-------------------+
 * |               timestamp                   |
 * +-----------+--------------+----------------+-------------------+
 *
 * fmt = 3  0 bytes
 */


#endif //RTP1078_RTMP_H
