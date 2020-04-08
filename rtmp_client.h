//
// Created by xushuyang on 2020-4-3.
//

#ifndef RTP1078_RTMP_CLIENT_H
#define RTP1078_RTMP_CLIENT_H

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include<boost/function.hpp>

#include "rtmp_packet_stream.h"

class tcp_session;

class rtmp_client
{
public:
	rtmp_client(boost::asio::io_service& io_service, tcp_session* session);

	void start();

	size_t send_video_or_audio_packet(uint8_t fm, uint32_t delta, uint8_t frame_type, const char* data, size_t size);
private:
	void handle_connected(const boost::system::error_code err);

	void do_handshake_c0c1();

	void do_rtmp_connect();

	void do_receive_rtmp_packet();

	void do_parse_rtmp_packet(const char* buf, std::size_t size);

	void do_handle_rtmp_set_ack_size(const char* buf, std::size_t size);

	void do_handle_rtmp_set_bandwidth(const char* buf, std::size_t size);

	void do_handle_rtmp_set_chunk_size(const char* buf, std::size_t size);

	void do_handle_rtmp_cmd_amf(const char* buf, std::size_t size);

	void do_send_acknowledgement();

	void do_send_acknowledgement_size();

	void do_send_create_stream();

	void do_send_fc_publish();

	void do_send_publish();

 private:
	tcp_session* session_;
	boost::asio::io_service& io_service_;
	boost::asio::ip::tcp::socket socket_;
	boost::asio::streambuf read_stream_;

	uint32_t window_size_;
	uint32_t acknowledgement_size_;
	uint32_t chunk_size_;

	uint32_t receive_bytes_count_;

	rtmp_packet_stream rtmp_output_stream_;
};

#endif //RTP1078_RTMP_CLIENT_H
