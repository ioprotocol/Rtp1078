//
// Created by xushuyang on 2020-4-1.
//

#ifndef RTP1078_TCP_SERVER_H
#define RTP1078_TCP_SERVER_H

#include <cstdlib>
#include <boost/log/trivial.hpp>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "tcp_session.h"

using boost::asio::ip::tcp;

class tcp_server {
public:
    tcp_server(boost::asio::io_service &io_service, short port)
            : io_service_(io_service), acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
        tcp_session *new_session = new tcp_session(io_service_);
        acceptor_.async_accept(new_session->socket(),
                               boost::bind(&tcp_server::handle_accept, this, new_session,
                                           boost::asio::placeholders::error));
    }

    void handle_accept(tcp_session *new_session, const boost::system::error_code &error) {
        if (!error) {
            BOOST_LOG_TRIVIAL(debug) << "jtt1078 client connected:"
                                     << new_session->socket().remote_endpoint().address().to_string() << ":"
                                     << new_session->socket().remote_endpoint().port() << "\n";
            new_session->start();
            new_session = new tcp_session(io_service_);
            acceptor_.async_accept(new_session->socket(),
                                   boost::bind(&tcp_server::handle_accept, this, new_session,
                                               boost::asio::placeholders::error));
        } else {
            delete new_session;
        }
    }

private:
    boost::asio::io_service &io_service_;
    tcp::acceptor acceptor_;
};


#endif //RTP1078_TCP_SERVER_H
