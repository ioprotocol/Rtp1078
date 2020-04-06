//
// Created by xushuyang on 2020-4-3.
//
#include <boost/log/trivial.hpp>

#include "rtmp_client.h"
#include "system.h"
#include "rtmp.h"
#include "common_utils.h"
#include "rtmp_s1_matcher.h"
#include "rtmp_chunk_matcher.h"

extern config_t global_config;

rtmp_client::rtmp_client(boost::asio::io_service& io_service) : socket_(io_service), chunk_size_(NGX_RTMP_DEFAULT_CHUNK_SIZE), rtmp_output_stream_(chunk_size_)
{
    cmd_connect_.app = "live";
    cmd_connect_.flashver = "FMLE/3.0 (compatible; FMSc/1.0)";
    cmd_connect_.tc_url = "rtmp://192.168.1.106:1935/live";
    cmd_connect_.audio_codecs = SUPPORT_SND_AAC;
    cmd_connect_.vidio_codecs = SUPPORT_VID_H264;
    cmd_connect_.capabilities = 0;
    cmd_connect_.vidio_function = 0;
    cmd_connect_.transaction_id = 0;
}

void rtmp_client::start(boost::function<void(const boost::system::error_code)> ready_handler)
{
    boost::asio::ip::tcp::endpoint rtmp_endpoint(boost::asio::ip::address::from_string(global_config.rtmp_svr), global_config.rtmp_port);
    socket_.async_connect(rtmp_endpoint, boost::bind(&rtmp_client::handle_connected, this, boost::asio::placeholders::error, ready_handler));
}

void rtmp_client::handle_connected(const boost::system::error_code err, boost::function<void(const boost::system::error_code)> ready_handler)
{
    if (!err)
    {
        BOOST_LOG_TRIVIAL(info) << "Proxy client connected!\n";
        do_handshake_c0c1(ready_handler);
    }
    else
    {
        ready_handler(err);
    }
}

void rtmp_client::do_handshake_c0c1(boost::function<void(const boost::system::error_code)> ready_handler)
{
    rtmp_output_stream_.create_c0c1_packet();
    // write c0,c1
    boost::asio::async_write(socket_, boost::asio::buffer(rtmp_output_stream_.data(), rtmp_output_stream_.size()), [this, ready_handler](boost::system::error_code err, size_t bytes_transferred)
    {
      if (!err)
      {
          BOOST_LOG_TRIVIAL(info) << "HANDSHAKE c0,c1 sent ! size:" << bytes_transferred << "\n";
          // read s0,s1,s2
          boost::asio::async_read_until(socket_, read_stream_, rtmp_s1_matcher(1 + RTMP_S1_LENGTH + RTMP_S2_LENGTH), [this, ready_handler](boost::system::error_code err, size_t bytes_transferred)
          {
            if (!err)
            {
                BOOST_LOG_TRIVIAL(info) << "HANDSHAKE s0,s1,s2 read ! size:" << bytes_transferred << "\n";
                const char* data_ptr = boost::asio::buffer_cast<const char*>(this->read_stream_.data());
                rtmp_output_stream_.create_c2_packet(data_ptr + 1, RTMP_S1_LENGTH);
                this->read_stream_.consume(bytes_transferred);
                boost::asio::async_write(socket_, boost::asio::buffer(rtmp_output_stream_.data(), rtmp_output_stream_.size()), [this, ready_handler](boost::system::error_code err, size_t bytes_transferred)
                {
                  if (!err)
                  {
                      BOOST_LOG_TRIVIAL(info) << "HANDSHAKE c2 send success !" << bytes_transferred << "\n";
                      do_rtmp_connect(ready_handler);
                  }
                  else
                  {
                      BOOST_LOG_TRIVIAL(error) << "HANDSHAKE c2 send failed!" << err.message() << "\n";
                      socket_.close();
                      ready_handler(err);
                  }
                });
            }
            else
            {
                BOOST_LOG_TRIVIAL(error) << "HANDSHAKE s0,s1 read err!" << err.message() << "\n";
                socket_.close();
                ready_handler(err);
            }
          });
      }
      else
      {
          BOOST_LOG_TRIVIAL(error) << "HANDSHAKE c0,c1 send err!" << err.message() << "\n";
          socket_.close();
          ready_handler(err);
      }
    });
}

void rtmp_client::do_rtmp_connect(boost::function<void(const boost::system::error_code)> ready_handler)
{
    rtmp_output_stream_.create_connect_packet(cmd_connect_);
    boost::asio::async_write(socket_, boost::asio::buffer(rtmp_output_stream_.data(), rtmp_output_stream_.size()), [this, ready_handler](boost::system::error_code err, size_t bytes_transferred)
    {
      if (!err)
      {
          BOOST_LOG_TRIVIAL(info) << "RTMP_CONNECT send success!" << bytes_transferred << "\n";
          boost::asio::async_read_until(socket_, read_stream_, rtmp_chunk_matcher(RTMP_S1_LENGTH), [this, ready_handler](boost::system::error_code err, size_t bytes_transferred)
          {
            if (!err)
            {
                BOOST_LOG_TRIVIAL(info) << "RTMP_CONNECT read success!" << bytes_transferred << "\n";
                const char* data_ptr = boost::asio::buffer_cast<const char*>(this->read_stream_.data());
                print_packet(data_ptr, bytes_transferred);
            }
            else
            {
                BOOST_LOG_TRIVIAL(error) << "RTMP_CONNECT read err!" << socket_.remote_endpoint() << err.message() << "\n";
            }
          });
      }
      else
      {
          BOOST_LOG_TRIVIAL(error) << "RTMP_CONNECT send err!" << err.message() << "\n";
          socket_.close();
          ready_handler(err);
      }
    });
}

