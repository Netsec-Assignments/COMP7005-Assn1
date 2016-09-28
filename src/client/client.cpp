#include <iostream>
#include <client/client.h>
#include <boost/filesystem.hpp>
#include <util/packet.hpp>
#include <util/file_transfer.hpp>

using boost::asio::io_service;
using boost::asio::ip::tcp;

client::client(io_service& service, std::string& host, std::string& storage_path)
: service_(service), socket_(service_), storage_path_(storage_path) {

    // Check whether the path is a directory, and if so, whether we have read-write access to it
    // throw invalid argument exception if either case is false
    if(!boost::filesystem::is_directory(storage_path_)) {
        throw std::invalid_argument("storage path isn't a directory");
    }
    if(!dir_is_read_write(storage_path)) {
        throw std::invalid_argument("can't read from and/or write to directory");
    }


    tcp::resolver resolver{service_};
    tcp::resolver::query query{host.c_str(), "7005"};
    auto endpoint_iterator = resolver.resolve(query);
    
    boost::asio::connect(socket_, endpoint_iterator);

}

void client::get(std::string& file_name) {
    boost::filesystem::path file_path(storage_path_);
    std::string actual_name(boost::filesystem::path(file_name).filename().c_str());
    file_path /= actual_name;

    std::ofstream file(file_path.c_str());
    if(!file) {
        std::cerr << "Error while creating/opening file " << file_name << std::endl;
        return;
    }    

    // Try to send a packet requesting the file
    get_packet g{actual_name};
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

void client::send(std::string& file_path) {
    boost::filesystem::path path(storage_path_);
    std::string name(boost::filesystem::path(file_path).filename().c_str());
    path /= file_path;

    if(!boost::filesystem::exists(path)) {
        std::cerr << "File \"" << path.c_str() << "\" doesn't exist!" << std::endl;
        return;
    } else if(boost::filesystem::is_directory(path)) {
        std::cerr << "File \"" << path.c_str() << "\" is a directory!" << std::endl;
        return;
    }

    // Open the file and get ready to send it
    boost::filesystem::ifstream file(path.c_str());
    if(!file) {
        std::cerr << "Couldn't open file \"" << path.c_str() << "\". Aborting send." << std::endl;
        return;
    }

    // Send a send_packet and the file to the server
    uint32_t size = (uint32_t)boost::filesystem::file_size(path);
    send_packet s{name, size};
    if(!s.send(socket_)) return;

    send_file(file, socket_);
}
