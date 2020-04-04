//
// Created by xushuyang on 2020-4-3.
//
#include <boost/log/trivial.hpp>

#include "rtmp_client.h"
#include "system.h"
#include "rtmp.h"

extern config_t global_config;

void rtmp_client::start(boost::function<void(const boost::system::error_code)> ready_handler) {
    boost::asio::ip::tcp::endpoint rtmp_endpoint(boost::asio::ip::address::from_string(global_config.rtmp_svr),
                                                 global_config.rtmp_port);
    socket_.async_connect(rtmp_endpoint,
                          boost::bind(&rtmp_client::handle_connected, this, boost::asio::placeholders::error,
                                      ready_handler));
}

void rtmp_client::handle_connected(const boost::system::error_code err,
                                   boost::function<void(const boost::system::error_code)> ready_handler) {
    if (!err) {
        BOOST_LOG_TRIVIAL(info) << "rtmp proxy connected!\n";
        do_handshake(ready_handler);
    } else {
        ready_handler(err);
    }
}

void rtmp_client::do_handshake(boost::function<void(const boost::system::error_code)> ready_handler) {
    data_[0] = C0;
    packet_c1_t *packet_c1 = reinterpret_cast<packet_c1_t *> (&data_[1]);
    packet_c1->time = 0;
    packet_c1->zero = 0;

    boost::asio::async_write(socket_,
                             boost::asio::buffer(data_, C1_LENGTH + 1),
                             [this, ready_handler](boost::system::error_code, size_t bytes_transferred) {
                             });
}
