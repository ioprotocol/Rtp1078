//
// Created by xushuyang on 2020-4-1.
//
#include <boost/log/trivial.hpp>

#include "rtmp.h"
#include "tcp_session.h"
#include "jtt1078_matcher.h"

void tcp_session::start() {
    rtmp_client_.start(boost::bind(&tcp_session::try_async_read, this, boost::asio::placeholders::error));
}

void tcp_session::try_async_read(const boost::system::error_code &error) {
    BOOST_LOG_TRIVIAL(info) << "try_async_read!\n";
    cmd_connect_.setName("connect");
    cmd_connect_.setApp("live");
    cmd_connect_.setFlashver("FMLE/3.0 (compatible; FMSc/1.0)");
    cmd_connect_.setCapabilities(0xe06d40);
    cmd_connect_.setAudioCodecs(0x9040);
    cmd_connect_.setVidioCodecs(0x6040);
    cmd_connect_.setVidioFunction(0xf03f);
    cmd_connect_.setTransactionId(0xf03f);
    cmd_connect_.setTcUrl("rtmp://192.168.1.106:1935/vod");

    rtmp_client_.do_rtmp_connect(cmd_connect_, [](boost::system::error_code err) {
        if(!err) {
        }
    });

    boost::asio::async_read_until(socket_, read_stream_, jtt1078_matcher(),
                                  boost::bind(&tcp_session::handle_read, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
}

void tcp_session::handle_read(const boost::system::error_code &error, size_t bytes_transferred) {
    if (!error) {
        handle_packet(bytes_transferred);
        try_async_read(error);
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


