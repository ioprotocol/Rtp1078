#include <cstdlib>
#include <iostream>

#include "tcp_server.h"

int main(int argc, char *argv[]) {
    std::string str = R"(\12333\ddd)";
    static_assert(sizeof(int) == 4, "must one");
    try {
        if (argc != 2) {
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        }

        boost::asio::io_service io_service;

        tcp_server s(io_service, std::atoi(argv[1]));

        io_service.run();
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}