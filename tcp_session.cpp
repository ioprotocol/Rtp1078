//
// Created by xushuyang on 2020-4-1.
//

#include "tcp_session.h"

void tcp_session::start() {
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
                            boost::bind(&tcp_session::handle_read, this,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
}

