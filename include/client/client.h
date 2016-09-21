#include <string>
#include <boost/asio.hpp>

class client {

public:
    client(boost::asio::io_service& service, std::string& host);

    void get(std::string& filename);
    void send(std::string& filename);

private:
    boost::asio::io_service& service_;
    boost::asio::ip::tcp::socket socket_;
};
