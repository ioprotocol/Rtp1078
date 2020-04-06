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
          BOOST_LOG_TRIVIAL(info) << "HANDSHAKE c0,c1 sent success! size:" << bytes_transferred << "\n";
          // read s0,s1,s2
          boost::asio::async_read_until(socket_, read_stream_, rtmp_s1_matcher(1 + RTMP_S1_LENGTH + RTMP_S2_LENGTH), [this, ready_handler](boost::system::error_code err, size_t bytes_transferred)
          {
            if (!err)
            {
                BOOST_LOG_TRIVIAL(info) << "HANDSHAKE s0,s1,s2 read success ! size:" << bytes_transferred << "\n";
                const char* data_ptr = boost::asio::buffer_cast<const char*>(this->read_stream_.data());
                rtmp_output_stream_.create_c2_packet(data_ptr + 1, RTMP_S1_LENGTH);
                this->read_stream_.consume(bytes_transferred);
                boost::asio::async_write(socket_, boost::asio::buffer(rtmp_output_stream_.data(), rtmp_output_stream_.size()), [this, ready_handler](boost::system::error_code err, size_t bytes_transferred)
                {
                  if (!err)
                  {
                      BOOST_LOG_TRIVIAL(info) << "HANDSHAKE c2 send success !" << bytes_transferred << "\n";
                  }
                  else
                  {
                      BOOST_LOG_TRIVIAL(error) << "HANDSHAKE c2 send failed!" << err.message() << "\n";
                  }
                  ready_handler(err);
                });
            }
            else
            {
                BOOST_LOG_TRIVIAL(error) << "HANDSHAKE s0,s1 read err!" << err.message() << "\n";
                ready_handler(err);
            }
          });
      }
      else
      {
          BOOST_LOG_TRIVIAL(error) << "HANDSHAKE c0,c1 send err!" << err.message() << "\n";
          ready_handler(err);
      }
    });
}

void rtmp_client::do_rtmp_connect(rtmp_cmd_connect_t& cmd, boost::function<void(const boost::system::error_code)> ready_handler)
{
    rtmp_output_stream_.create_connect_packet(cmd);
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
      }
      ready_handler(err);
    });
}

