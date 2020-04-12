//
// Created by xushuyang on 2020-4-1.
//
#include <boost/log/trivial.hpp>
#include <sys/time.h>

#include "tcp_session.h"
#include "jtt1078_matcher.h"
#include "common_utils.h"

void tcp_session::start()
{
	read_stream_.consume(bytes_transferred_);
	boost::asio::async_read_until(socket_, read_stream_, jtt1078_matcher(),
			[this](boost::system::error_code err, size_t size)
			{
				if (!err)
				{
					this->bytes_transferred_ = size;
					handle_jtt1078_packet();
				}
				else
				{
					BOOST_LOG_TRIVIAL(info) << "Start read err!" << err.message() << "\n";
					delete this;
				}
			});
}

void tcp_session::begin_proxy()
{
	BOOST_LOG_TRIVIAL(info) << "Proxy client connected, begin proxy data!" << "\n";
	proxy_client_status_ = proxy_connected;
	handle_jtt1078_packet();
}

void tcp_session::pause_proxy(uint32_t err)
{
	BOOST_LOG_TRIVIAL(info) << "Proxy client err, pause to proxy data!" << "\n";
//	proxy_client_status_ = proxy_disconnect;
	read_stream_.consume(bytes_transferred_);
	socket_.close();
}

void tcp_session::handle_jtt1078_packet()
{
	if (proxy_client_status_ != proxy_connected)
	{
		BOOST_LOG_TRIVIAL(info) << "Proxy client is disconnect, try to connect!" << "\n";
		rtmp_client_.start();
		return;
	}

	const char* data_ptr = boost::asio::buffer_cast<const char*>(this->read_stream_.data());

	struct timeval tv;
	gettimeofday(&tv,NULL);
	long ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	uint64_t timestamp = read_uint64(data_ptr + 16);
	int dts = read_uint16(data_ptr + 24);
	int pts = ms / 100;
	size_t data_size = read_uint16(data_ptr + 28);
	int spit_type = *(data_ptr + 15) & 0xF;
	int vtype = (*(data_ptr + 15) >> 4) & 0xF;

	if (vtype>= 3)
	{
		start();
		return;
	}
	// H.264码流分Annex-B和AVCC两种格式。 目前采用的是Annex-B, 拆开H264码流的每一帧,每帧码流以 0x00 00 00 01分割
	const char* frame_begin = data_ptr + 30;
	h264_stream_.append(const_cast<char*>(data_ptr + 30), data_size);
	BOOST_LOG_TRIVIAL(info) << "read:" << bytes_transferred_ << " data_size:" << data_size << "\n";
	print_packet("after append", h264_stream_.data(), h264_stream_.size());

	if (spit_type == 1 || spit_type == 3)
	{
		start();
		return;
	}

	char* h264_raw = h264_stream_.data();
	size_t nalu_size = h264_stream_.size();
//	while(h264_stream_.find_nalu(&h264_raw, &nalu_size))
//	{
		double fps = 25;
		// @remark, to decode the file.
		char* p = h264_raw;
		for (; p < h264_raw + nalu_size;)
		{
			// @remark, read a frame from file buffer.
			char* data = NULL;
			int size = 0;
			int nb_start_code = 0;
			if (read_h264_frame(h264_raw, (int)nalu_size, &p, &nb_start_code, fps, &data, &size, &dts, &pts) < 0)
			{
				break;
			}
			handle_h264_frames(dts, pts, data, size);
		}
	h264_stream_.reset();
	start();
}

void tcp_session::handle_h264_frames(uint32_t dts, uint32_t pts, const char* frames, size_t frames_size)
{
	stream_buffer* stream = new stream_buffer(const_cast<char*>(frames), frames_size);

	// use the last error
	// @see https://github.com/ossrs/srs/issues/203
	// @see https://github.com/ossrs/srs/issues/204

	// send each frame.
	while (!stream->empty())
	{
		char* frame = NULL;
		int frame_size = 0;
		if (annexb_demux(stream, &frame, &frame_size) != 0)
		{
			return;
		}

		// ignore invalid frame,
		// atleast 1bytes for SPS to decode the type
		if (frame_size <= 0)
		{
			continue;
		}

		// it may be return error, but we must process all packets.
		handle_h264_frame(dts, pts, frame, frame_size);
	}
}

void tcp_session::handle_h264_frame(uint32_t dts, uint32_t pts, const char* data, size_t size)
{
	// for sps
	if (h264_is_sps(data, size))
	{
		std::string sps;
		if (h264_sps_demux(data, size, sps) != 0)
		{
			return;
		}

		if (ctx_.h264_sps == sps)
		{
			return;
		}
		ctx_.h264_sps_changed = true;
		ctx_.h264_sps = sps;
		return;
	}

	// for pps
	if (h264_is_pps(data, size))
	{
		std::string pps;
		if (h264_pps_demux(data, size, pps) != 0)
		{
			return;
		}

		if (ctx_.h264_pps == pps)
		{
			return;
		}
		ctx_.h264_pps_changed = true;
		ctx_.h264_pps = pps;
		return;
	}

	// ignore others.
	// 5bits, 7.3.1 NAL unit syntax,
	// ISO_IEC_14496-10-AVC-2003.pdf, page 44.
	//  7: SPS, 8: PPS, 5: I Frame, 1: P Frame, 9: AUD
	uint32_t nut = (uint32_t)(data[0] & 0x1f);
	if (nut != 7 && nut != 8 && nut != 5 && nut != 1 && nut != 9)
	{
		return;
	}

	// send pps+sps before ipb frames when sps/pps changed.
	if (ctx_.h264_pps_changed || ctx_.h264_sps_changed)
	{
		// h264 raw to h264 packet.
		std::string sh;
		if ((h264_mux_sequence_header(ctx_.h264_sps, ctx_.h264_pps, dts, pts, sh)) != 0)
		{
			return;
		}

		// h264 packet to flv packet.
//		int8_t frame_type = SrsVideoAvcFrameTypeKeyFrame;
//		int8_t avc_packet_type = SrsVideoAvcFrameTraitSequenceHeader;
		int8_t frame_type = 1;
		int8_t avc_packet_type = 0;
		char* flv = NULL;
		int nb_flv = 0;
		if (h264_mux_avc2flv(sh, frame_type, avc_packet_type, dts, pts, &flv, &nb_flv) != 0)
		{
			return;
		}

		// reset sps and pps.
		ctx_.h264_sps_changed = false;
		ctx_.h264_pps_changed = false;
		ctx_.h264_sps_pps_sent = true;

		// the timestamp in rtmp message header is dts.
		rtmp_client_.send_video_or_audio_packet(0, dts, 0, flv, nb_flv);
	}

	// when sps or pps not sent, ignore the packet.
	// @see https://github.com/ossrs/srs/issues/203
	if (!ctx_.h264_sps_pps_sent)
	{
		return;
	}

	// 5bits, 7.3.1 NAL unit syntax,
	// ISO_IEC_14496-10-AVC-2003.pdf, page 44.
	//  5: I Frame, 1: P/B Frame
	// @remark we already group sps/pps to sequence header frame;
	//      for I/P NALU, we send them in isolate frame, each NALU in a frame;
	//      for other NALU, for example, AUD/SEI, we just ignore them, because
	//      AUD used in annexb to split frame, while SEI generally we can ignore it.
	// TODO: maybe we should group all NALUs split by AUD to a frame.
//	if (nut != SrsAvcNaluTypeIDR && nut != SrsAvcNaluTypeNonIDR) {
	if (nut != 5 && nut != 1)
	{
		return;
	}

	// for IDR frame, the frame is keyframe.
//	uint32_t frame_type = SrsVideoAvcFrameTypeInterFrame;
	uint32_t frame_type = 2;
	if (nut == 5)
	{
		frame_type = 1;
	}

	std::string ibp;
	if (h264_mux_ipb_frame(data, size, ibp) != 0)
	{
		return;
	}

//	int8_t avc_packet_type = SrsVideoAvcFrameTraitNALU;
	int8_t avc_packet_type = 1;
	char* flv = NULL;
	int nb_flv = 0;
	if (h264_mux_avc2flv(ibp, frame_type, avc_packet_type, dts, pts, &flv, &nb_flv) != 0)
	{
		return;
	}

	// the timestamp in rtmp message header is dts.
//	return srs_rtmp_write_packet(context, SRS_RTMP_TYPE_VIDEO, timestamp, flv, nb_flv);
	rtmp_client_.send_video_or_audio_packet(0, dts, 1, flv, nb_flv);
}

void tcp_session::handle_session_error(const boost::system::error_code& error)
{
	BOOST_LOG_TRIVIAL(info) << "handle_session_error:" << error.message();
	// when read from the socket_, some error occur.
	socket_.close();
	delete this;
}


