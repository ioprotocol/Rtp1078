//
// Created by xushuyang on 2020-4-1.
//

#include "tcp_session.h"
#include "jtt1078_matcher.h"

void tcp_session::start() {
    boost::asio::async_read_until(socket_, read_stream_, jtt1078_matcher(),
                                  boost::bind(&tcp_session::handle_read, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
}

void tcp_session::handle_read(const boost::system::error_code &error, size_t bytes_transferred) {
    if (!error) {
        handle_packet(bytes_transferred);
        start();
    } else {
        // when read from the socket_, some error occur.
        socket_.close();
        delete this;
        handle_close(error);
    }
}

void tcp_session::handle_packet(size_t bytes_transferred) {
    std::cout << "socket read bytes:" << bytes_transferred << std::endl;
    read_stream_.consume(bytes_transferred);
}

void tcp_session::handle_close(const boost::system::error_code &error) {
    std::cout << "socket read error:" << error << std::endl;

}


