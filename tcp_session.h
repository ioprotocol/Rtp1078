//
// Created by xushuyang on 2020-4-1.
//

#ifndef RTP1078_TCP_SESSION_H
#define RTP1078_TCP_SESSION_H

#include <boost/asio.hpp>

#include "rtmp_client.h"
#include "h264_stream.h"

using boost::asio::ip::tcp;

class tcp_session
{
 public:
	tcp_session(boost::asio::io_service& io_service)
		: socket_(io_service), rtmp_client_(io_service, this)
	{
		proxy_client_status_ = proxy_disconnect;
		ctx_.app = "live";
		ctx_.flashver = "WIN 15,0,0,239";
		ctx_.tc_url = "rtmp://127.0.0.1/live";
		ctx_.audio_codecs = 0xeeab40;
		ctx_.vidio_codecs = 0x806f40;
		ctx_.capabilities = 0xe06d40;
		ctx_.vidio_function = 0xf03f;
		ctx_.transaction_id = 0xf03f;
		ctx_.name = "test?vhost=127.0.0.1";
	}

	tcp::socket& socket()
	{
		return socket_;
	}

	rtmp_context_t& ctx() {
		return ctx_;
	}

	void start();

	void begin_proxy();

	void pause_proxy(uint32_t err);

	void handle_jtt1078_packet();

	void handle_h264_frames(uint32_t dts, uint32_t pts, const char* data, size_t size);

	void handle_h264_frame(uint32_t dts, uint32_t pts, const char* data, size_t size);

	void handle_session_error(const boost::system::error_code& error);
 private:
	tcp::socket socket_;
	enum
	{
		max_length = 1024
	};
	char data_[max_length];
	boost::asio::streambuf read_stream_;
	h264_stream h264_stream_;
	size_t bytes_transferred_;

	rtmp_client rtmp_client_;

	enum
	{
		proxy_disconnect,
		proxy_connected,
		proxy_err
	};

	uint32_t proxy_client_status_;

	rtmp_context_t ctx_;
};

#endif //RTP1078_TCP_SESSION_H
