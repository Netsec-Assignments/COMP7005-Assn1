#include <stdexcept>
#include <iostream>
#include <util/file_transfer.hpp>
#include <util/packet.hpp>
#include <server/server.h>

#define PORT 7005

server::server(boost::asio::io_service& service, std::string& storage_path)
: service_(service), storage_path_(storage_path), files_{}, acceptor_(service) {
    // Make sure the path is a directory
    if(!boost::filesystem::is_directory(storage_path)) {
        throw std::invalid_argument("storage path isn't a directory");
    }
    
    // Make sure that we can read/write from/to the directory
    // Apparently the only portable, guaranteed way to do this is to actually read/write from/to the directory
    // Go figure

    // Check readability
    try {
        auto it = boost::filesystem::directory_iterator(storage_path);

        if(!boost::filesystem::is_empty(storage_path)) {
            for(; it != boost::filesystem::directory_iterator(); ++it) {
                boost::filesystem::path p = it->path();
                files_.emplace(p.filename().c_str());
            }
        }
    } catch(boost::filesystem::filesystem_error& err) {
        throw std::invalid_argument("storage path isn't readable");
    }

    // Check writeabiliy (partially stolen from http://en.cppreference.com/w/cpp/io/c/remove)
    boost::filesystem::path test_path(storage_path);
    test_path /= "file1.txt";

    bool ok = static_cast<bool>(std::ofstream(test_path.c_str()).put('a'));
    if(!ok) {
        throw std::invalid_argument("storage path isn't writeable");
    } else {
        std::remove(test_path.c_str());
    }

    // Initialise the acceptor
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), PORT);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
}

void server::handle_send_request(boost::asio::ip::tcp::socket& sock) {
    send_packet s{sock};
    boost::filesystem::path file_path(storage_path_);

    std::cout << "Client is sending file " << s.name << std::endl;

    file_path /= s.name;
    std::ofstream file(file_path.c_str());
    if(!file) {
        error_packet ep{"Couldn't open file for writing."};
        ep.send(sock);
        return;
    }

    if(!receive_file(file, s.file_size, sock)) {
        file.close();
        std::remove(file_path.c_str());
        std::cout << "File was not stored." << std::endl;
    } else {
        file.close();
        std::cout << "Successfully received file and stored at " << file_path.c_str() << '.' << std::endl;
    }
}

void server::start() {
    for(;;) {
        boost::asio::ip::tcp::socket sock(service_);
        acceptor_.listen();
        acceptor_.accept(sock);
        
        // Read packets until the client disconnects
        packet_type pt;
        bool connected = true;
        while(connected) {
            try {
                boost::asio::read(sock, boost::asio::buffer(&pt, sizeof(packet_type)));
                switch(pt) {
                    case SEND:
                        handle_send_request(sock);
                        break;
                    case GET:
                        break;
                    default:
                        break;
                }
            } catch(boost::system::system_error& err) {
                // Client disconnected
                if(boost::asio::error::eof == err.code() ||
                   boost::asio::error::connection_reset == err.code()) {
                    std::cout << "Client disconnected. Waiting for new client..." << std::endl;
                    connected = false;
                } else {
                    std::cerr << "Error while reading from socket: " << err.what() << std::endl;
                    std::cerr << "Exiting..." << std::endl;
                    return;
                }
                    
            }
        }
    }
}
