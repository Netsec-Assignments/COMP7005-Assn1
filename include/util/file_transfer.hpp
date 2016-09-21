#pragma once

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <fstream>
#include <string>

const std::size_t BUF_SIZE = 1024;

/**
 * Sends the given file to the remote host.
 *
 * @param file Handle to the file to send.
 * @param sock The socket over which to send the file.
 *
 * @return true if the file was successfully sent; false if not.
 */
bool send_file(std::ifstream& file, boost::asio::ip::tcp::socket& sock) {
    char buf[BUF_SIZE];
    boost::system::error_code error;

    // Read the file in 1KB chunks and send them to the other host
    while(file.read(buf, BUF_SIZE)) {
        if (file.gcount() < 0) {
            std::cerr << "Error while reading file" << std::endl;
            return false;
        }
        boost::asio::write(sock, boost::asio::buffer(buf, file.gcount()), boost::asio::transfer_all(), error);
        if (error) {
            std::cerr << "Error while sending file: " << error << std::endl;
            return false;
        }
    }
}

/**
 * Receives a file from the remote host, writing it to out_path.
 *
 * @param file      The file to store the contents.
 * @param file_size The size (in bytes) of the file being transmitted.
 * @param sock      The socket over which to receive the file.
 */
bool receive_file(std::ofstream& file, unsigned int file_size, boost::asio::ip::tcp::socket& sock) {
    char buf[BUF_SIZE];
    size_t total_bytes_read = 0;
    boost::system::error_code error;
    
    bool file_had_error = false;

    // Read from the socket in 1KB chunks and write them to the file
    while(total_bytes_read < file_size) {
        size_t bytes_read = boost::asio::read(sock, boost::asio::buffer(buf, BUF_SIZE), error);
        total_bytes_read += bytes_read;
        if(error) {
            std::cerr << "Error while receiving file: " << error << std::endl;
            return false;
        }
        
        // We still want to read everything from the server even if there was a file error
        // so just don't write to the file if that happened
        if(!file_had_error) {
            file.write(buf, bytes_read);
            if(file.gcount <= 0) {
                std::cerr << "Error while writing to file" << std::endl;
                return false;
            }
        }
    }

    return true;
}
