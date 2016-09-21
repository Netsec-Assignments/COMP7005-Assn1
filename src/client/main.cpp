#include <iostream>
#include <string>
#include <vector>
#include <client/client.h>
#include <util/packet.hpp>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

bool parse_command(std::string& command, packet_type& op, std::string& filename) {
    std::vector<std::string> command_parts;
    boost::split(command_parts, command, boost::is_any_of(" "));

    if(command_parts.size() < 2 || (command_parts[0] != "GET" && command_parts[0] != "SEND")) {
        std::cout << command << std::endl;
        return false;
    } else {
        // Figure out the operation
        op = command_parts[0] == "GET" ? GET : SEND;

        // Remove GET/SEND from the string and copy the remainder of the string into filename
        command_parts.erase(command_parts.begin(), command_parts.begin() + 1);
        filename = boost::algorithm::join(command_parts, "");

        return true;
    }
}

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "usage: " << argv[0] << " [host name]" << std::endl;
        return 1;
    }

    boost::asio::io_service service;
    std::string host_name(argv[1]);

    try {
        client c(service, host_name);

        std::cout << "Connected to " << host_name << std::endl;
        std::cout << std::endl;

        std::cout << "Type SEND [filename] to send a file to the server." << std::endl;
        std::cout << "Type GET [filename] to get a file from the server." << std::endl;
        std::cout << "Type Ctrl + D to quit" << std::endl;
        std::cout << std::endl;

        while(true) {
            std::string command;
            std::cout << "Enter a command: ";
            std::getline(std::cin, command);

            // Check whether they've quit
            if(!std::cin) {
                break;
            }

            packet_type op;
            std::string filename;
            if(!parse_command(command, op, filename)) {
                std::cout << "Command format: SEND|GET [filename]" << std::endl;
            } else {
                if(op == SEND) {
                    c.send(filename);
                } else {
                    c.get(filename);
                }
            }

        }
    } catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "See ya!" << std::endl;

    return 0;
}
