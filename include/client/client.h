/* ========================================================================
   $HEADER FILE
   $File: client.h $
   $Program: $
   $Developer: Shane Spoor $
   $Created On: 2016/09/22 $
   $Description: $
   $    Header file for the client
   $Revisions: $
   ======================================================================== */
#pragma once

#include <string>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <util/boost_net_interface.hpp>

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
    // Never thought I'd actually rely on construction order... We need the sockets to be constructed first
    boost::asio::io_service& service_;
    boost::asio::ip::tcp::socket control_socket_;
    boost_net_interface control_interface_;
    boost::filesystem::path storage_path_;

    // Attempts to accept an incoming data port connection on out. Throws on failure.
    void accept_data_channel_conn(boost::asio::ip::tcp::socket& out);
};
