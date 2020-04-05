//
// Created by xushuyang on 2020-4-1.
//

#ifndef RTP1078_TCP_SESSION_H
#define RTP1078_TCP_SESSION_H

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <iostream>

#include "rtmp_client.h"
#include "rtmp_cmd_connect.h"

using boost::asio::ip::tcp;

class tcp_session {
public:
    tcp_session(boost::asio::io_service &io_service)
            : socket_(io_service), rtmp_client_(io_service) {
    }

    tcp::socket &socket() {
        return socket_;
    }

    void start();

    void try_async_read(const boost::system::error_code &error);

    void handle_read(const boost::system::error_code &error, size_t bytes_transferred);

    void handle_packet(size_t bytes_transferred);

    void handle_close(const boost::system::error_code &error);
private:
    tcp::socket socket_;
    enum {
        max_length = 1024
    };
    char data_[max_length];
    boost::asio::streambuf read_stream_;

    rtmp_client rtmp_client_;
    rtmp_cmd_connect cmd_connect_;
};


#endif //RTP1078_TCP_SESSION_H
