#include <iostream>
#include <client/client.h>
#include <boost/filesystem.hpp>
#include <util/packet.hpp>
#include <util/file_transfer.hpp>

using boost::asio::io_service;
using boost::asio::ip::tcp;

client::client(io_service& service, std::string& host)
: service_(service), socket_(service_) {
    tcp::resolver resolver{service_};
    tcp::resolver::query query{host.c_str(), "7005"};
    auto endpoint_iterator = resolver.resolve(query);
    
    boost::asio::connect(socket_, endpoint_iterator);
}

void client::get(std::string& file_name) {
    std::ofstream file(file_name);
    if(!file) {
        std::cerr << "Error while creating/opening file " << file_name << std::endl;
        return;
    }    

    // Try to send a packet requesting the file
    get_packet g{file_name};
    if(!g.send(socket_)) return;

    packet_type pt;
    boost::asio::read(socket_, boost::asio::buffer(&pt, sizeof(packet_type)));
    if(pt == SEND) {
        send_packet sp(socket_); // Deseriliase the send packet
        receive_file(file, sp.file_size, socket_);
    } else if(pt == ERROR) {
        error_packet ep(socket_);
        std::cerr << "Server reported error: " << ep.err << std::endl;
    }
}

void client::send(std::string& file_name) {
    if(!boost::filesystem::exists(file_name)) {
        std::cerr << "File \"" << file_name << "\" doesn't exist!" << std::endl;
        return;
    } else if(boost::filesystem::is_directory(file_name)) {
        std::cerr << "File \"" << file_name << "\" is a directory!" << std::endl;
        return;
    }

    // Open the file and get ready to send it
    boost::filesystem::ifstream file(file_name);
    if(!file) {
        std::cerr << "Couldn't open file \"" << file_name << "\". Aborting send." << std::endl;
        return;
    }

    // Send a send_packet and the file to the server
    uint32_t size = (uint32_t)boost::filesystem::file_size(file_name);
    std::string file_name_stripped(boost::filesystem::path(file_name).filename().c_str());
    send_packet s{file_name_stripped, size};
    if(!s.send(socket_)) return;

    send_file(file, socket_);
}
