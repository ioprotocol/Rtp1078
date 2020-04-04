#include <iostream>
#include <boost/log/trivial.hpp>

#include "tcp_server.h"
#include "system_init.h"

config_t global_config;

int main(int argc, char *argv[]) {
    init();

    try {
        boost::asio::io_service io_service;

        tcp_server s(io_service, global_config.bind_port);

        io_service.run();
    }
    catch (std::exception &e) {
        BOOST_LOG_TRIVIAL(error) << "Exception: " << e.what() << "\n";
    }

    return 0;
}