#ifndef BOOST_NET_INTERFACE_H
#define BOOST_NET_INTERFACE_H

#include <boost/asio.hpp>
#include <util/net_interface.h>

class boost_net_interface : public net_interface {
public:
    boost_net_interface(boost::asio::ip::tcp::socket& sock)
    : sock_(sock) {}
    
    // What's a little duplicated code anyway
    virtual void send(void* buf, size_t size) {
        try {
            boost::asio::write(sock_, boost::asio::buffer(buf, size));
        } catch(boost::system::system_error& e) {
            if(boost::asio::error::eof == e.code()) {
                throw net_interface::error(e.what(), error_code::eof);
            } else if(boost::asio::error::connection_reset == e.code()) {
                throw net_interface::error(e.what(), error_code::reset);
            } else {
                throw net_interface::error(e.what(), error_code::other);
            }
        }
    }

    virtual void receive(void* buf, size_t size) {
        try {
            boost::asio::read(sock_, boost::asio::buffer(buf, size));
        } catch(boost::system::system_error& e) {
            if(boost::asio::error::eof == e.code()) {
                throw net_interface::error(e.what(), error_code::eof);
            } else if(boost::asio::error::connection_reset == e.code()) {
                throw net_interface::error(e.what(), error_code::reset);
            } else {
                throw net_interface::error(e.what(), error_code::other);
            }
        }
    }

    const boost::asio::ip::tcp::socket& get_socket() {
        return sock_;
    }

private:
    boost::asio::ip::tcp::socket& sock_;
};

#endif // BOOST_NET_INTERFACE
