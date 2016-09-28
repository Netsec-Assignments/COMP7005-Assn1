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
    if(!dir_is_read_write(storage_path)) {
        throw std::invalid_argument("can't read from and/or write to directory");
    }

    boost::filesystem::directory_iterator it(storage_path);
    if(!boost::filesystem::is_empty(storage_path)) {
        for(; it != boost::filesystem::directory_iterator(); ++it) {
            boost::filesystem::path p = it->path();
            files_.emplace(p.filename().c_str());
        }
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
        files_.emplace(s.name);
    }
}

void server::handle_get_request(boost::asio::ip::tcp::socket& sock) {
    get_packet g{sock};
    
    auto it = files_.find(g.name);

    // Check whether the file exists; if not, send back an error packet
    if(it == files_.end()) {
        std::string s("Couldn't find file ");
        std::ostringstream oss(s);
        oss << g.name << '.';
        error_packet e{oss.str()};
        
        e.send(sock);
    } else {
        boost::filesystem::path file_path(storage_path_);
        file_path /= g.name;

        // Send back a send_packet so that the client knows we're sending the file
        std::uint32_t size = boost::filesystem::file_size(file_path);
        send_packet s{std::string(file_path.c_str()), size};
        s.send(sock);
        
        std::ifstream file(file_path.c_str());
        send_file(file, sock);
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
                        handle_get_request(sock);
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
