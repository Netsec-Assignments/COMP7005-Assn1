#pragma once

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <unordered_set>
#include <util/net_interface.h>

class server {

public:
    /**
     * Creates a new server with the given io_service and file storage path.
     * The server automatically tries to bind to the first available interface on creation.
     *
     * @param service      The io_service to communicate with the OS TCP/IP stack.
     * @param storage_path The path in which to retrieve and store files.
     *
     * @throws boost::system::error_code if binding/accept socket creation fails,
     *         std::exception if storage_path isn't a directory or doesn't have
     *         read/write access.
     */
    server(boost::asio::io_service& service, std::string& storage_path);
    
    // No copy constructor (Note: might do a move constructor if I'm feeling ambitious, but we don't really need one)
    server(server& other) = delete;

    /**
     * Causes the server to start listening for connections and serving clients.
     */
    void start();

private:
    boost::asio::io_service& service_;
    boost::asio::ip::tcp::acceptor acceptor_;

    boost::filesystem::path storage_path_;

    // A list of the files in the storage directory so that it doesn't have to be searched every time
    std::unordered_set<std::string> files_;

    void handle_send_request(net_interface& control_interface);
    void handle_get_request(net_interface& control_interface);

    // Attempts to connect to control_sock.remote_endpoint(); throws on failure, otherwise out will be a socket connected to port 7006
    void connect_to_data_channel(const boost::asio::ip::tcp::socket& control_sock, boost::asio::ip::tcp::socket& out);
};
