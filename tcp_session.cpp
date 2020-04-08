//
// Created by xushuyang on 2020-4-1.
//
#include <boost/log/trivial.hpp>

#include "tcp_session.h"
#include "jtt1078_matcher.h"
#include "common_utils.h"

void tcp_session::start()
{
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
}

void tcp_session::handle_jtt1078_packet()
{
	if (proxy_client_status_ != proxy_connected)
	{
		BOOST_LOG_TRIVIAL(info) << "Proxy client is disconnect, try to connect!" << "\n";
		rtmp_client_.start();
		return;
	}

	BOOST_LOG_TRIVIAL(info) << "handle_packet" << "\n";
	const char* data_ptr = boost::asio::buffer_cast<const char*>(this->read_stream_.data());

	uint8_t frame_type = (*(data_ptr + 15) & 0xF0) >> 4;
	uint64_t timestamp = read_uint64(data_ptr + 16);
	uint16_t iframe_timestamp = read_uint16(data_ptr + 24);
	uint16_t pframe_timestamp = read_uint16(data_ptr + 26);
	size_t data_size = read_uint16(data_ptr + 28);

	// H.264码流分Annex-B和AVCC两种格式。 目前采用的是Annex-B, 拆开H264码流的每一帧,每帧码流以 0x00 00 00 01分割
	const char* frame_begin = data_ptr + 30;
	for (int i = 0; i < data_size;)
	{
		if (i + 4 < data_size)
		{
			if (read_uint32(frame_begin + i) == 1)
			{
				i += 4;
				int j = i + 1;
				for (j = i + 1; j < data_size; j++)
				{
					if (j + 4 < data_size)
					{
						if (read_uint32(frame_begin + j) == 1)
						{
							break;
						}
					}
				}

				handle_h264_frame(frame_begin + i, j - i);
				i = j;
				continue;
			}
		}
		i++;
	}
	start();
}

void tcp_session::handle_h264_frame(const char* data, size_t size)
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
		return ;
	}

	// send pps+sps before ipb frames when sps/pps changed.
	if (ctx_.h264_pps_changed || ctx_.h264_sps_changed)
	{

	}
}


void tcp_session::handle_session_error(const boost::system::error_code& error)
{
	BOOST_LOG_TRIVIAL(info) << "handle_session_error:" << error.message();
	// when read from the socket_, some error occur.
	socket_.close();
	delete this;
}


