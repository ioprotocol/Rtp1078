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
        do_handshake_c0c1(ready_handler);
    } else {
        ready_handler(err);
    }
}

void rtmp_client::do_handshake_c0c1(boost::function<void(const boost::system::error_code)> ready_handler) {
    data_[0] = C0;
    packet_c1_t *packet_c1 = reinterpret_cast<packet_c1_t *> (&data_[1]);
    packet_c1->time = 0;
    packet_c1->zero = 0;

    for (size_t i = 8; i <= C1_LENGTH; i++) {
        packet_c1->payload[i - 8] = (i * 111) & 0xFF;
    }
    // write c0,c1
    boost::asio::async_write(socket_, boost::asio::buffer(data_, C1_LENGTH + 1), [this, ready_handler](boost::system::error_code err, size_t bytes_transferred) {
                                 if (!err) {
                                     BOOST_LOG_TRIVIAL(info) << "Handshake c0,c1 sent success!\n";
                                     // read s0,s1
                                     boost::asio::async_read_until(socket_, read_stream_, rtmp_s1_matcher(1 + S1_LENGTH + S2_LENGTH), [this, ready_handler](boost::system::error_code err, size_t bytes_transferred) {
                                                                       if (!err) {
                                                                           BOOST_LOG_TRIVIAL(info) << "Handshake s0,s1,s2 read success !" << bytes_transferred << "\n";
                                                                           const char *data_ptr = boost::asio::buffer_cast< const char* >(this->read_stream_.data());
                                                                           print_packet(data_ptr, bytes_transferred);
                                                                           for (size_t i = 1; i <= S1_LENGTH ; i++) {
                                                                               this->data_[i-1] = *(data_ptr + i);
                                                                           }
                                                                           this->read_stream_.consume(bytes_transferred);
                                                                           boost::asio::async_write(socket_, boost::asio::buffer(data_, S1_LENGTH), [this, ready_handler](boost::system::error_code err, size_t bytes_transferred) {
                                                                               if (!err) {
                                                                                   BOOST_LOG_TRIVIAL(info) << "Handshake c2 send success !" << bytes_transferred << "\n";
                                                                               } else {
                                                                                   BOOST_LOG_TRIVIAL(error) << "Handshake c2 send failed!" << err.message() << "\n";
                                                                               }
                                                                               ready_handler(err);
                                                                           });
                                                                       } else {
                                                                           BOOST_LOG_TRIVIAL(error) << "Handshake s0,s1 read err!" << err.message() << "\n";
                                                                           ready_handler(err);
                                                                       }
                                                                   });
                                 } else {
                                     BOOST_LOG_TRIVIAL(error) << "Handshake c0,c1 send err!" << err.message() << "\n";
                                     ready_handler(err);
                                 }
                             });
}

void rtmp_client::do_rtmp_connect(rtmp_cmd_connect &cmd_connect, boost::function<void(const boost::system::error_code)> ready_handler) {
    size_t size = encode_rtmp_cmd_connect(cmd_connect);
    print_packet(data_, size);
    BOOST_LOG_TRIVIAL(info) << "data_!" << size << "\n";
    boost::asio::async_write(socket_, boost::asio::buffer(data_, size), [this, ready_handler](boost::system::error_code err, size_t bytes_transferred) {
        if(!err) {
            BOOST_LOG_TRIVIAL(info) << "do_rtmp_connect send success!" << bytes_transferred << "\n";
            boost::asio::streambuf b;
            boost::asio::async_read_until(socket_, b, rtmp_chunk_matcher(S1_LENGTH), [this, ready_handler](boost::system::error_code err, size_t bytes_transferred) {
                if(!err) {
                    BOOST_LOG_TRIVIAL(info) << "do_rtmp_connect read success!" << bytes_transferred << "\n";
                    const char *data_ptr = boost::asio::buffer_cast< const char* >(this->read_stream_.data());
                    print_packet(data_ptr, bytes_transferred);
                } else {
                    BOOST_LOG_TRIVIAL(error) << "do_rtmp_connect read err!" << err.message() << "\n";
                }
            });
        } else {
            BOOST_LOG_TRIVIAL(error) << "do_rtmp_connect send err!" << err.message() << "\n";
        }
        ready_handler(err);
    });
}

size_t rtmp_client::encode_rtmp_cmd_connect(rtmp_cmd_connect &cmd_connect) {
    data_[0] = 3;
    // timestamp
    char* p = &data_[1];
    write_uint24(&p, 0);
    // payload_size
    p = &data_[4];
    write_uint24(&p, 0);
    // message type id
    data_[7] = NGX_RTMP_MSG_AMF_CMD;
    // message stream id
    p = &data_[8];
    write_uint32(&p, 0);
    // payload
    p = &data_[12];
    *p++ = NGX_RTMP_AMF_STRING;
    write_string(&p, cmd_connect.getName());
    *p++ = NGX_RTMP_AMF_NUMBER;
    write_uint64(&p, BYTE_ORDER_SWAP64(cmd_connect.getTransactionId()));
    *p++ = NGX_RTMP_AMF_OBJECT;
    write_string(&p, std::string("objectEncoding"));
    *p++ = NGX_RTMP_AMF_NUMBER;
    write_uint64(&p, 0);
    write_string(&p, std::string("fpad"));
    *p++ = NGX_RTMP_AMF_BOOLEAN;
    *p++ = cmd_connect.getFpad();
    write_string(&p, std::string("flashVer"));
    *p++ = NGX_RTMP_AMF_STRING;
    write_string(&p, cmd_connect.getFlashver());
    write_string(&p, std::string("capabilities"));
    *p++ = NGX_RTMP_AMF_NUMBER;
    write_uint64(&p, BYTE_ORDER_SWAP64(cmd_connect.getCapabilities()));
    write_string(&p, std::string("audioCodecs"));
    *p++ = NGX_RTMP_AMF_NUMBER;
    write_uint64(&p, BYTE_ORDER_SWAP64(cmd_connect.getAudioCodecs()));
    write_string(&p, std::string("videoCodecs"));
    *p++ = NGX_RTMP_AMF_NUMBER;
    write_uint64(&p, BYTE_ORDER_SWAP64(cmd_connect.getVidioCodecs()));
    write_string(&p, std::string("videoFunction"));
    *p++ = NGX_RTMP_AMF_NUMBER;
    write_uint64(&p, BYTE_ORDER_SWAP64(cmd_connect.getVidioFunction()));
    write_string(&p, std::string("swfUrl"));
    *p++ = NGX_RTMP_AMF_NULL;
    write_string(&p, std::string("pageUrl"));
    *p++ = NGX_RTMP_AMF_NULL;
    write_string(&p, std::string("app"));
    *p++ = NGX_RTMP_AMF_STRING;
    write_string(&p, cmd_connect.getApp());
    write_string(&p, std::string("tcUrl"));
    *p++ = NGX_RTMP_AMF_STRING;
    write_string(&p, cmd_connect.getTcUrl());
    write_uint24(&p, NGX_RTMP_AMF_END);

    // update payload size
    char* s = &data_[4];
    write_uint24(&s, p - &data_[12]);
    // split 3 chunk
    char* m = &data_[12];
    char* n = p;
    while(n - m > 128) {
        m = m + 128;
        n = n + 1;
        while (n > m) {
            *n = *(n-1);
            n--;
        }
        *m = (3 << 6) | 3;
        m++;
        p++;
        n = p;
    }
    return p - &data_[0];
}
