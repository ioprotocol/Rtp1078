//
// Created by xushuyang on 2020-4-1.
//

#ifndef RTP1078_TCP_SESSION_H
#define RTP1078_TCP_SESSION_H

#include <boost/bind.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class tcp_session {
public:
    tcp_session(boost::asio::io_service &io_service)
            : socket_(io_service) {
    }

    tcp::socket &socket() {
        return socket_;
    }

    void start();

    void handle_read(const boost::system::error_code &error, size_t bytes_transferred) {
        if (!error) {
            boost::asio::async_write(socket_,
                                     boost::asio::buffer(data_, bytes_transferred),
                                     boost::bind(&tcp_session::handle_write, this,
                                                 boost::asio::placeholders::error));
        } else {
            delete this;
        }
    }

    void handle_write(const boost::system::error_code &error) {
        if (!error) {
            socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                    boost::bind(&tcp_session::handle_read, this,
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
        } else {
            delete this;
        }
    }

private:
    tcp::socket socket_;
    enum {
        max_length = 1024
    };
    char data_[max_length];
};


#endif //RTP1078_TCP_SESSION_H
