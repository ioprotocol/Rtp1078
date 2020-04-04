//
// Created by xushuyang on 2020-4-3.
//

#ifndef RTP1078_RTMP_CLIENT_H
#define RTP1078_RTMP_CLIENT_H

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include<boost/function.hpp>

class rtmp_client {
public:
    rtmp_client(boost::asio::io_service &io_service) : socket_(io_service) {}

    void start(boost::function<void(const boost::system::error_code)> ready_handler);

    void do_handshake_c0c1(boost::function<void(const boost::system::error_code)> ready_handler);

    void do_handshake_c2(boost::function<void(const boost::system::error_code)> ready_handler);

    void handle_connected(const boost::system::error_code err,
                          boost::function<void(const boost::system::error_code)> connected_handler);

private:
    boost::asio::ip::tcp::socket socket_;

    boost::asio::streambuf read_stream_;

    enum {
        max_length = 4096
    };
    char data_[max_length];
};


#endif //RTP1078_RTMP_CLIENT_H
