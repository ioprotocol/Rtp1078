//
// Created by xushuyang on 2020-4-1.
//

#ifndef RTP1078_TCP_SESSION_H
#define RTP1078_TCP_SESSION_H

#include <boost/asio.hpp>

#include "rtmp_client.h"

using boost::asio::ip::tcp;

class tcp_session
{
 public:
	tcp_session(boost::asio::io_service& io_service)
		: socket_(io_service), rtmp_client_(io_service, this)
	{
		proxy_client_status_ = proxy_disconnect;
		ctx_.app = "live";
		ctx_.flashver = "FMLE/3.0 (compatible; FMSc/1.0)";
		ctx_.tc_url = "rtmp://192.168.1.106:1935/live";
		ctx_.audio_codecs = SUPPORT_SND_AAC;
		ctx_.vidio_codecs = SUPPORT_VID_H264;
		ctx_.capabilities = 0;
		ctx_.vidio_function = 0;
		ctx_.transaction_id = 1935;
		ctx_.name = "test";
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

	void handle_h264_frame(const char* data, size_t size);

	void handle_session_error(const boost::system::error_code& error);
 private:
	tcp::socket socket_;
	enum
	{
		max_length = 1024
	};
	char data_[max_length];
	boost::asio::streambuf read_stream_;
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
