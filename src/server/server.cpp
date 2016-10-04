#include <stdexcept>
#include <iostream>
#include <util/file_transfer.hpp>
#include <util/packet.hpp>
#include <server/server.h>
#include <util/ports.h>
#include <util/boost_net_interface.hpp>

namespace fs = boost::filesystem;
using namespace boost::asio::ip;
using namespace boost;

server::server(asio::io_service& service, std::string& storage_path)
: service_(service), storage_path_(storage_path), files_{}, acceptor_(service) {
    // Make sure the path is a directory
    if(!fs::is_directory(storage_path)) {
        throw std::invalid_argument("storage path isn't a directory");
    }
    
    // Make sure that we can read/write from/to the directory
    if(!dir_is_read_write(storage_path)) {
        throw std::invalid_argument("can't read from and/or write to directory");
    }

    fs::directory_iterator it(storage_path);
    if(!fs::is_empty(storage_path)) {
        for(; it != fs::directory_iterator(); ++it) {
            fs::path p = it->path();
            files_.emplace(p.filename().c_str());
        }
    }

    // Initialise the acceptor
    tcp::endpoint endpoint(tcp::v4(), CONTROL_PORT);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
}

void server::connect_to_data_channel(const boost::asio::ip::tcp::socket& control_socket, boost::asio::ip::tcp::socket& out) {
        // Connect to the client's data port (7006)
        std::ostringstream oss;
        oss << DATA_PORT;

        tcp::endpoint remote_end = control_socket.remote_endpoint();
        std::string address = remote_end.address().to_string();

        // Open a connection on port 7006 to the client to send off the file
        tcp::resolver resolver{service_};
        tcp::resolver::query query{remote_end.address().to_string(), oss.str()};
        auto endpoint_iterator = resolver.resolve(query);
        
        asio::connect(out, endpoint_iterator);
        std::cout << "Connected to client on data channel (port " << DATA_PORT << ")." << std::endl;
}

void server::handle_send_request(net_interface& control_interface) {
    send_packet s{control_interface};
    fs::path file_path(storage_path_);

    std::cout << "Client is sending file " << s.name << std::endl;

    file_path /= s.name;
    std::ofstream file(file_path.c_str());
    if(!file) {
        std::string err("Couldn't open file for writing.");
        error_packet ep{err};
        ep.send(control_interface);
        std::cerr << err << std::endl;
        return;
    }

    tcp::socket data_sock(service_);
    try {
        connect_to_data_channel(((boost_net_interface*)&control_interface)->get_socket(), data_sock);
    } catch(std::exception& e) {
        std::cerr << "Error while initiating connection to client on data port." << std::endl;
        return;
    }

    boost_net_interface data_interface(data_sock);

    if(!receive_file(file, s.file_size, data_interface)) {
        file.close();
        std::remove(file_path.c_str());
        std::cout << "File was not stored." << std::endl;
    } else {
        file.close();
        std::cout << "Successfully received file and stored at " << file_path.c_str() << '.' << std::endl;
        files_.emplace(s.name);
    }
}

void server::handle_get_request(net_interface& control_interface) {
    get_packet g{control_interface};
    
    auto it = files_.find(g.name);
    
    std::cout << "Attempting to send file " << g.name << "..." << std::endl;

    // Check whether the file exists; if not, send back an error packet
    if(it == files_.end()) {
        std::ostringstream oss;
        oss << "Couldn't find file " << g.name << '.';
        error_packet e{oss.str()};
        
        std::cout << oss.str() << std::endl;

        if(!e.send(control_interface)) {
            std::cerr << "Transmission of error packet failed." << std::endl;
        }
        
    } else {
        fs::path file_path(storage_path_);
        file_path /= g.name;

        // Send back a send_packet so that the client knows we're sending the file
        std::uint32_t size = fs::file_size(file_path);
        send_packet s{std::string(file_path.c_str()), size};
        s.send(control_interface);
        
        std::ifstream file(file_path.c_str());
        
        tcp::socket data_sock(service_);
        try {
            connect_to_data_channel(((boost_net_interface*)&control_interface)->get_socket(), data_sock);
        } catch(std::exception& e) {
            std::cerr << "Error while initiating connection to client on data port." << std::endl;
            return;
        }

        boost_net_interface data_interface(data_sock);

        try {
            send_file(file, data_interface);
            std::cout << "Successfully sent file." << std::endl;
        } catch(net_interface::error& e) {
            std::cerr << "File was not sent successfully. Error: " << e.what() << std::endl;
        }
    }
}

void server::start() {
    for(;;) {
        // Accept client's connection on control port (7005)
        tcp::socket control_sock(service_);
        acceptor_.listen();
        acceptor_.accept(control_sock);      

        std::cout << "Accepted connection from " << control_sock.remote_endpoint().address().to_string() << " on control channel (port " << CONTROL_PORT << ")." << std::endl;
        
        // Wrap the socket objects in a net_interface; we'll later swap this out for an
        // interface that performs additional packetizing for the final project
        boost_net_interface control_interface(control_sock);

        // Read packets from the control socket until the client disconnects
        packet_type pt;
        bool connected = true;
        while(connected) {
            try {
                control_interface.receive(&pt, sizeof(packet_type));
                switch(pt) {
                    case SEND:
                        handle_send_request(control_interface);
                        break;
                    case GET:
                        handle_get_request(control_interface);
                        break;
                    default:
                        break;
                }
            } catch(net_interface::error& err) {
                // Client disconnected
                if(net_interface::error_code::eof == err.code() ||
                   net_interface::error_code::reset == err.code()) {
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
