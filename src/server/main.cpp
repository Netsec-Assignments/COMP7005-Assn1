#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <server/server.h>

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cout << "usage: " << argv[0] << " [storage directory]" << std::endl;
        return 1;
    }

    boost::asio::io_service service;
    
    try {
        std::string p(argv[1]);
        // Might figure out how to thread this later, but for now just Ctrl + C out of it
        server s(service, p);
        s.start();
    } catch(boost::system::error_code& ec) {
        std::cerr << "Error: " << ec << std::endl;
        return 1;
    }

    return 0;
}
