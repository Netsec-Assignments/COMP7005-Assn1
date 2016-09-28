#pragma once

#include <string>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

class client {

public:
    client(boost::asio::io_service& service, std::string& host, std::string& storage_path);

    /**
     * Requests a file with the given name from the server.
     *
     * @param file_name The name of the remote file.
     */
    void get(std::string& file_name);

    /**
     * Sends a file with the given path to the server. Note that file paths are relative to storage_path.
     *
     * @param file_path The path to the file to send.
     */
    void send(std::string& file_path);

private:
    boost::asio::io_service& service_;
    boost::asio::ip::tcp::socket socket_;
    boost::filesystem::path storage_path_;
};
