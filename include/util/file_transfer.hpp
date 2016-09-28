#pragma once

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <string>
#include <stdexcept>

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
    while(!file.eof()) {
        file.read(buf, BUF_SIZE);
        if (file.gcount() < 0) {
            std::cerr << "Error while reading file" << std::endl;
            return false;
        } else if(file.gcount() == 0) {
            break;
        }
        boost::asio::write(sock, boost::asio::buffer(buf, file.gcount()), boost::asio::transfer_all(), error);
        if (error) {
            std::cerr << "Error while sending file: " << error << std::endl;
            return false;
        }
    }
    return true;
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

    // Figure out how many full BUF_SIZE chunks we'll read and
    // how many bytes will be left over
    size_t chunks = file_size / BUF_SIZE;
    size_t last_bytes = file_size % BUF_SIZE;
    size_t num_reads = chunks + (last_bytes ? 1 : 0);

    boost::system::error_code error;
    bool file_had_error = false;

    for(int i = 0; i < num_reads; ++i) {
        // Hackily check whether we need to read a whole BUF_SIZE chunk or just the last bytes
        size_t bytes_to_read = last_bytes && (i == num_reads - 1) ? last_bytes : BUF_SIZE;
        boost::asio::read(sock, boost::asio::buffer(buf, BUF_SIZE), boost::asio::transfer_exactly(bytes_to_read), error);
        if(error) {
            std::cerr << "Error while receiving file: " << error << std::endl;
            return false;
        }
        
        // We still want to read everything from the server even if there was a file error
        // so just don't write to the file if that happened
        if(!file_had_error) {
            file.write(buf, bytes_to_read);
            if(!file) {
                std::cerr << "Error while writing to file" << std::endl;
                return false;
            }
        }
    }

    return true;
}

/**
 * Checks whether storage_path is readable and writeable.
 *
 * @param storage_path The path to check for read/write access.
 * @throws std::invalid_argument if the path isn't readable or writeable.
 */
bool dir_is_read_write(std::string& storage_path) {
    // Apparently the only portable, guaranteed way to do this is to actually read/write from/to the directory        
    // Go figure

    // Check readability
    try {
        auto it = boost::filesystem::directory_iterator(storage_path);
    } catch(boost::filesystem::filesystem_error& err) {
        return false;
    }

    // Check writeabiliy (partially stolen from http://en.cppreference.com/w/cpp/io/c/remove)
    boost::filesystem::path test_path(storage_path);
    test_path /= "file1.txt";

    bool ok = static_cast<bool>(std::ofstream(test_path.c_str()).put('a'));
    if(!ok) {
        return false;
    } else {
        std::remove(test_path.c_str());
    }

    return true;
}
