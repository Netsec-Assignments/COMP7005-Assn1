/* ========================================================================
   $File: client.cpp $
   $Program: $
   $Developer: Shane Spoor $
   $Created On: 2016/09/22 $
   $Description: $
   $     This is the client portion of the program 
   $Revisions: $
   ======================================================================== */
#include <iostream>
#include <client/client.h>
#include <boost/filesystem.hpp>
#include <util/packet.hpp>
#include <util/file_transfer.hpp>
#include <util/ports.h>

using boost::asio::io_service;
using boost::asio::ip::tcp;

/* ========================================================================
   $ FUNCTION
   $ Name: client() $
   $ Prototype: (io_service& service, std::string& host, std::string& storage_path): service_(service), control_socket_(service_), control_interface_(control_socket_), storage_path_(storage_path) { $
   $ Params: 
   $    name: The name of the program $
   $ Description:  $
   $    The constructor for the client 
   ======================================================================== */
client::client(io_service& service, std::string& host, std::string& storage_path)
        : service_(service), control_socket_(service_), control_interface_(control_socket_), storage_path_(storage_path) {

    // Check whether the path is a directory, and if so, whether we have read-write access to it
    // throw invalid argument exception if either case is false
    if(!boost::filesystem::is_directory(storage_path_)) {
        throw std::invalid_argument("storage path isn't a directory");
    }
    if(!dir_is_read_write(storage_path)) {
        throw std::invalid_argument("can't read from and/or write to directory");
    }

    // Connect the control socket on port 7005
    std::ostringstream oss;
    oss << CONTROL_PORT;

    tcp::resolver resolver{service_};
    tcp::resolver::query query{host.c_str(), oss.str()};
    auto endpoint_iterator = resolver.resolve(query);
    
    boost::asio::connect(control_socket_, endpoint_iterator);

    std::cout << "Connected to server on control channel (port " << CONTROL_PORT << ")." << std::endl;
}

void client::accept_data_channel_conn(boost::asio::ip::tcp::socket& out) {
    // Connect the data socket on port 7006
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), DATA_PORT);
    boost::asio::ip::tcp::acceptor a(service_);

    a.open(endpoint.protocol());
    a.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    a.bind(endpoint);
    a.listen();
    
    a.accept(out);
    std::cout << "Received connection from server on data channel (port " << DATA_PORT << ")." << std::endl;
}

/* ========================================================================
   $ FUNCTION
   $ Name: client::get $
   $ Prototype: void client::get(std::string& file_name) { $
   $ Params: 
   $    file_name: The name of the file to get from the server $
   $ Description:  $
   $   get the file from the server
   ======================================================================== */
void client::get(std::string& file_name) {
    boost::filesystem::path file_path(storage_path_);
    std::string actual_name(boost::filesystem::path(file_name).filename().c_str());
    file_path /= actual_name;

    std::ofstream file(file_path.c_str());
    if(!file) {
        std::cerr << "Error while creating/opening file " << file_name << std::endl;
        return;
    }    

    std::cout << "Attempting to retrieve file " << actual_name << '.' << std::endl;

    // Try to send a packet requesting the file
    get_packet g{actual_name};
    if(!g.send(control_interface_)) return;

    packet_type pt;
    control_interface_.receive(&pt, sizeof(packet_type));
    if(pt == SEND) {
        send_packet sp(control_interface_); // Deseriliase the send packet

        boost::asio::ip::tcp::socket data_sock(service_);
        try {
            accept_data_channel_conn(data_sock);
        } catch(std::exception& e) {
            std::cerr << "Error while accepting server data connection: " << e.what() << std::endl;
            return;
        }
        boost_net_interface data_interface(data_sock);

        receive_file(file, sp.file_size, data_interface);
        std::cout << "Successfully retrieved file." << std::endl;
    } else if(pt == ERROR) {
        error_packet ep(control_interface_);
        std::cerr << "Server reported error: " << ep.err << std::endl;
        std::remove(file_path.c_str());
    }

    // Data socket will be closed upon leaving this function.
}

/* ========================================================================
   $ FUNCTION
   $ Name: client::send $
   $ Prototype: void client::send(std::string& file_path) { $
   $ Params: 
   $    file_path: The path  of the file $
   $ Description:  $
   $   Send a file to the server
   ======================================================================== */
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

    std::cout << "Attempting to send file " << path.c_str() << '.' << std::endl;

    // Send a send_packet and the file to the server
    uint32_t size = (uint32_t)boost::filesystem::file_size(path);
    send_packet s{name, size};
    if(!s.send(control_interface_)) { 
        return;
    }

    boost::asio::ip::tcp::socket data_sock(service_);
    try {
        accept_data_channel_conn(data_sock);
    } catch(std::exception& e) {
        std::cerr << "Error while accepting server data connection: " << e.what() << std::endl;
        return;
    }
    boost_net_interface data_interface(data_sock);

    if(send_file(file, data_interface)) {
        std::cout << "Successfully sent file." << std::endl;
    } else {
        std::cout << "Sending file was unsuccessful." << std::endl;
    }
}
