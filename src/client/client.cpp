#include <client.h>
#include <boost/filesystem.hpp>

using boost::asio::io_service;
using boost::asio::ip;
using namespace boost::filesystem;

client::client(io_service& service, std::string& host)
: service_(service), socket_(service_) {
    tcp::resolver resolver{service_};
    tpc::resolver::query query{host.c_str(), "7005"};
    auto endpoint_iterator = resolver.resolve(query);
    
    boost::asio::connect(socket, endpoint_iterator);
}

void client::send(std::string& filename) {
    

}
