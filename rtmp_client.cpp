//
// Created by xushuyang on 2020-4-3.
//
#include <boost/log/trivial.hpp>

#include "rtmp_client.h"
#include "system.h"
#include "rtmp.h"

extern config_t global_config;

void rtmp_client::start(boost::function<void(const boost::system::error_code)> ready_handler) {
    boost::asio::ip::tcp::endpoint rtmp_endpoint(boost::asio::ip::address::from_string(global_config.rtmp_svr), global_config.rtmp_port);
    socket_.async_connect(rtmp_endpoint, boost::bind(&rtmp_client::handle_connected, this, boost::asio::placeholders::error, ready_handler));
}

void rtmp_client::handle_connected(const boost::system::error_code err, boost::function<void(const boost::system::error_code)> ready_handler) {
    if(!err) {
        BOOST_LOG_TRIVIAL(info) << "rtmp proxy connected!\n";
        do_handshake(ready_handler);
    } else {
        ready_handler(err);
    }
}

void rtmp_client::do_handshake(boost::function<void(const boost::system::error_code)> ready_handler) {
    boost::asio::async_write(socket_, boost::asio::buffer(write_buf_, 1537), );
}
