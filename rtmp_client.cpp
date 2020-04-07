//
// Created by xushuyang on 2020-4-3.
//
#include <boost/log/trivial.hpp>
#include <cstring>

#include "tcp_session.h"
#include "system.h"
#include "rtmp.h"
#include "rtmp_s1_matcher.h"
#include "rtmp_chunk_matcher.h"
#include "rtmp_client.h"

extern config_t global_config;

rtmp_client::rtmp_client(boost::asio::io_service& io_service, tcp_session* session)
	: socket_(io_service), io_service_(io_service), session_(session),
	  chunk_size_(NGX_RTMP_DEFAULT_CHUNK_SIZE), rtmp_output_stream_(chunk_size_)
{
	receive_bytes_count_ = 0;
	window_size_ = NGX_RTMP_MAX_WINDOW_SIZE;
	acknowledgement_size_ = NGX_RTMP_MAX_WINDOW_SIZE;
	cmd_connect_.app = "live";
	cmd_connect_.flashver = "FMLE/3.0 (compatible; FMSc/1.0)";
	cmd_connect_.tc_url = "rtmp://192.168.1.106:1935/live";
	cmd_connect_.audio_codecs = SUPPORT_SND_AAC;
	cmd_connect_.vidio_codecs = SUPPORT_VID_H264;
	cmd_connect_.capabilities = 0;
	cmd_connect_.vidio_function = 0;
	cmd_connect_.transaction_id = 0;
	cmd_connect_.name = "test";
}

void rtmp_client::start()
{
	boost::asio::ip::tcp::endpoint rtmp_endpoint(boost::asio::ip::address::from_string(global_config.rtmp_svr), global_config.rtmp_port);
	socket_.async_connect(rtmp_endpoint, boost::bind(&rtmp_client::handle_connected, this, boost::asio::placeholders::error));
}

void rtmp_client::handle_connected(const boost::system::error_code err)
{
	if (!err)
	{
		BOOST_LOG_TRIVIAL(info) << "Proxy client connected!\n";
		do_handshake_c0c1();
	}
	else
	{
		session_->handle_proxy(err);
	}
}

void rtmp_client::do_handshake_c0c1()
{
	rtmp_output_stream_.create_c0c1_packet();
	// write c0,c1
	boost::asio::async_write(socket_, boost::asio::buffer(rtmp_output_stream_.data(), rtmp_output_stream_.size()), [this](boost::system::error_code err, size_t bytes_transferred)
	{
	  if (!err)
	  {
		  BOOST_LOG_TRIVIAL(info) << "HANDSHAKE c0,c1 sent ! size:" << bytes_transferred << "\n";
		  // read s0,s1,s2
		  boost::asio::async_read_until(socket_, read_stream_, rtmp_s1_matcher(1 + RTMP_S1_LENGTH + RTMP_S2_LENGTH), [this](boost::system::error_code err, size_t bytes_transferred)
		  {
			if (!err)
			{
				BOOST_LOG_TRIVIAL(info) << "HANDSHAKE s0,s1,s2 read ! size:" << bytes_transferred << "\n";
				const char* data_ptr = boost::asio::buffer_cast<const char*>(this->read_stream_.data());
				rtmp_output_stream_.create_c2_packet(data_ptr + 1, RTMP_S1_LENGTH);
				this->read_stream_.consume(bytes_transferred);
				boost::asio::async_write(socket_, boost::asio::buffer(rtmp_output_stream_.data(), rtmp_output_stream_.size()), [this](boost::system::error_code err, size_t bytes_transferred)
				{
				  if (!err)
				  {
					  BOOST_LOG_TRIVIAL(info) << "HANDSHAKE c2 send success !" << bytes_transferred << "\n";
					  do_rtmp_connect();
				  }
				  else
				  {
					  BOOST_LOG_TRIVIAL(error) << "HANDSHAKE c2 send failed!" << err.message() << "\n";
					  socket_.close();
					  session_->handle_proxy(err);
				  }
				});
			}
			else
			{
				BOOST_LOG_TRIVIAL(error) << "HANDSHAKE s0,s1 read err!" << err.message() << "\n";
				socket_.close();
				session_->handle_proxy(err);
			}
		  });
	  }
	  else
	  {
		  BOOST_LOG_TRIVIAL(error) << "HANDSHAKE c0,c1 send err!" << err.message() << "\n";
		  socket_.close();
		  session_->handle_proxy(err);
	  }
	});
}

void rtmp_client::do_rtmp_connect()
{
	rtmp_output_stream_.create_connect_packet(cmd_connect_);
	boost::asio::async_write(socket_, boost::asio::buffer(rtmp_output_stream_.data(), rtmp_output_stream_.size()), [this](boost::system::error_code err, size_t bytes_transferred)
	{
	  if (!err)
	  {
		  BOOST_LOG_TRIVIAL(info) << "RTMP_CONNECT send success!" << bytes_transferred << "\n";
		  do_receive_rtmp_packet();
	  }
	  else
	  {
		  BOOST_LOG_TRIVIAL(error) << "RTMP_CONNECT send err!" << err.message() << "\n";
		  socket_.close();
		  session_->handle_proxy(err);
	  }
	});
}

void rtmp_client::do_receive_rtmp_packet()
{
	boost::asio::async_read_until(socket_, read_stream_, rtmp_chunk_matcher(chunk_size_), [this](boost::system::error_code err, size_t bytes_transferred)
	{
	  if (!err)
	  {
		  const char* data_ptr = boost::asio::buffer_cast<const char*>(this->read_stream_.data());
		  do_parse_rtmp_packet(data_ptr, bytes_transferred);
		  this->read_stream_.consume(bytes_transferred);
		  this->do_receive_rtmp_packet();
	  }
	  else
	  {
		  BOOST_LOG_TRIVIAL(error) << "do_receive_rtmp_packet err!" << socket_.remote_endpoint() << err.message() << "\n";
		  socket_.close();
		  session_->handle_proxy(err);
	  }
	});
}

void rtmp_client::do_parse_rtmp_packet(const char* buf, std::size_t size)
{
	receive_bytes_count_ += size;

	uint32_t message_type = rtmp_message_type(buf, size);
	switch (message_type)
	{
	case NGX_RTMP_MSG_CHUNK_SIZE:
		do_handle_rtmp_set_chunk_size(buf, size);
		break;
	case NGX_RTMP_MSG_ABORT:
		BOOST_LOG_TRIVIAL(info) << "NGX_RTMP_MSG_ABORT !" << "\n";
		break;
	case NGX_RTMP_MSG_ACK:
		BOOST_LOG_TRIVIAL(info) << "NGX_RTMP_MSG_ACK !" << "\n";
		break;
	case NGX_RTMP_MSG_USER:
		BOOST_LOG_TRIVIAL(info) << "NGX_RTMP_MSG_USER !" << "\n";
		break;
	case NGX_RTMP_MSG_ACK_SIZE:
		do_handle_rtmp_set_ack_size(buf, size);
		break;
	case NGX_RTMP_MSG_BANDWIDTH:
		do_handle_rtmp_set_bandwidth(buf, size);
		break;
	case NGX_RTMP_MSG_EDGE:
		BOOST_LOG_TRIVIAL(info) << "NGX_RTMP_MSG_EDGE !" << "\n";
		break;
	case NGX_RTMP_MSG_AUDIO:
		BOOST_LOG_TRIVIAL(info) << "NGX_RTMP_MSG_AUDIO !" << "\n";
		break;
	case NGX_RTMP_MSG_VIDEO:
		BOOST_LOG_TRIVIAL(info) << "NGX_RTMP_MSG_VIDEO !" << "\n";
		break;
	case NGX_RTMP_MSG_AMF3_META:
		BOOST_LOG_TRIVIAL(info) << "NGX_RTMP_MSG_AMF3_META !" << "\n";
		break;
	case NGX_RTMP_MSG_AMF3_SHARED:
		BOOST_LOG_TRIVIAL(info) << "NGX_RTMP_MSG_AMF3_SHARED !" << "\n";
		break;
	case NGX_RTMP_MSG_AMF3_CMD:
		BOOST_LOG_TRIVIAL(info) << "NGX_RTMP_MSG_AMF3_CMD !" << "\n";
		break;
	case NGX_RTMP_MSG_AMF_META:
		BOOST_LOG_TRIVIAL(info) << "NGX_RTMP_MSG_AMF_META !" << "\n";
		break;
	case NGX_RTMP_MSG_AMF_SHARED:
		BOOST_LOG_TRIVIAL(info) << "NGX_RTMP_MSG_AMF_SHARED !" << "\n";
		break;
	case NGX_RTMP_MSG_AMF_CMD:
		do_handle_rtmp_cmd_amf(buf, size);
		break;
	case NGX_RTMP_MSG_AGGREGATE:
		BOOST_LOG_TRIVIAL(info) << "NGX_RTMP_MSG_AGGREGATE !" << "\n";
		break;
	case NGX_RTMP_MSG_UNDEFINED:
		BOOST_LOG_TRIVIAL(error) << "NGX_RTMP_MSG_UNDEFINED !" << "\n";
		break;
	default:
		BOOST_LOG_TRIVIAL(error) << "NGX_RTMP_MSG_UNDEFINED ---!" << "\n";
		break;
	}

	if (receive_bytes_count_ > acknowledgement_size_)
	{
		this->do_send_acknowledgement();
	}
}

void rtmp_client::do_handle_rtmp_set_ack_size(const char* buf, std::size_t size)
{
	uint32_t head_size = rtmp_header_size(*buf);
	uint32_t new_ack_size = read_uint32(buf + head_size);
	BOOST_LOG_TRIVIAL(info) << "set acknowledgement_size_ from:" << acknowledgement_size_ << " to:" << new_ack_size << "\n";
	this->acknowledgement_size_ = new_ack_size;
	this->do_send_acknowledgement();
}

void rtmp_client::do_handle_rtmp_set_bandwidth(const char* buf, std::size_t size)
{
	uint32_t head_size = rtmp_header_size(*buf);
	uint32_t new_window_size = read_uint32(buf + head_size);
	BOOST_LOG_TRIVIAL(info) << "window_size_ from: " << window_size_ << " to: " << new_window_size << "\n";
	this->window_size_ = new_window_size;
	this->do_send_acknowledgement_size();
}

void rtmp_client::do_handle_rtmp_set_chunk_size(const char* buf, std::size_t size)
{
	uint32_t head_size = rtmp_header_size(*buf);
	uint32_t new_chunk_size = read_uint32(buf + head_size);
	BOOST_LOG_TRIVIAL(info) << "chunk_size_ from: " << chunk_size_ << " to: " << new_chunk_size << "\n";
	this->chunk_size_ = new_chunk_size;
	this->rtmp_output_stream_.set_chunk_size(this->chunk_size_);
}

void rtmp_client::do_handle_rtmp_cmd_amf(const char* buf, std::size_t size)
{
	uint32_t head_size = rtmp_header_size(*buf);

	uint8_t amf_type = buf[head_size];
	if (amf_type == NGX_RTMP_AMF_STRING)
	{
		// connect response
		size_t str_size = read_uint16(&buf[head_size + 1]);
		if (!std::strncmp(&buf[head_size + 3], "_result", str_size))
		{
			if (search_amf_tree(buf, size, "NetConnection.Connect.Success"))
			{
				do_send_fc_publish();
			}
		}
		if (!std::strncmp(&buf[head_size + 3], "onStatus", str_size))
		{
			if (search_amf_tree(buf, size, "NetStream.Publish.Start"))
			{
				BOOST_LOG_TRIVIAL(info) << "NetStream.Publish.Start" << "\n";
			}
		}
	}
}

void rtmp_client::do_send_acknowledgement()
{
	rtmp_output_stream_.create_acknowledgement(receive_bytes_count_);
	boost::asio::async_write(socket_, boost::asio::buffer(rtmp_output_stream_.data(), rtmp_output_stream_.size()), [this](boost::system::error_code err, size_t bytes_transferred)
	{
	  receive_bytes_count_ = 0;
	  if (err)
	  {
		  BOOST_LOG_TRIVIAL(error) << "do_send_acknowledgement send err!" << err.message() << "\n";
	  }
	});
}

void rtmp_client::do_send_acknowledgement_size()
{
	rtmp_output_stream_.create_acknowledgement_window_size(window_size_);
	boost::asio::async_write(socket_, boost::asio::buffer(rtmp_output_stream_.data(), rtmp_output_stream_.size()), [this](boost::system::error_code err, size_t bytes_transferred)
	{
	  if (err)
	  {
		  BOOST_LOG_TRIVIAL(error) << "do_send_acknowledgement_size send err!" << err.message() << "\n";
	  }
	});
}

void rtmp_client::do_send_fc_publish()
{
	rtmp_output_stream_.create_fc_publish_packet(cmd_connect_.name);
	boost::asio::async_write(socket_, boost::asio::buffer(rtmp_output_stream_.data(), rtmp_output_stream_.size()), [this](boost::system::error_code err, size_t bytes_transferred)
	{
	  if (err)
	  {
		  BOOST_LOG_TRIVIAL(error) << "do_send_acknowledgement_size send err!" << err.message() << "\n";
	  }
	  else
	  {
		  do_send_publish();
	  }
	});
}

void rtmp_client::do_send_publish()
{
	rtmp_output_stream_.create_publish_packet(cmd_connect_.app, cmd_connect_.name);
	boost::asio::async_write(socket_, boost::asio::buffer(rtmp_output_stream_.data(), rtmp_output_stream_.size()), [this](boost::system::error_code err, size_t bytes_transferred)
	{
	  if (err)
	  {
		  BOOST_LOG_TRIVIAL(error) << "do_send_acknowledgement_size send err!" << err.message() << "\n";
	  }
	});
}
