#include <iostream>
#include <client/client.h>
#include <boost/filesystem.hpp>
#include <util/protocol.hpp>

using boost::asio::io_service;
using boost::asio::ip::tcp;
using namespace boost::filesystem;

client::client(io_service& service, std::string& host)
: service_(service), socket_(service_) {
    tcp::resolver resolver{service_};
    tcp::resolver::query query{host.c_str(), "7000"};
    auto endpoint_iterator = resolver.resolve(query);
    
    boost::asio::connect(socket_, endpoint_iterator);
}

void client::get(std::string& filename) {
    // Do stuff
}

void client::send(std::string& filename) {
    if(!exists(filename)) {
        std::cerr << "File \"" << filename << "\" doesn't exist!" << std::endl;
        return;
    }

    uint32_t size = (uint32_t)file_size(filename);
    request_packet r{SEND, size};        
}
