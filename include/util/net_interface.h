/* ========================================================================
   $HEADER FILE
   $File: net_interface.h $
   $Program: $
   $Developer: Shane Spoor $
   $Created On: 2016/09/22 $
   $Description: $
   $    net_interface header file
   $Revisions: $
   ======================================================================== */
#ifndef NET_INTERFACE_H
#define NET_INTERFACE_H

#include <stdexcept>

class net_interface {

public:
    /**
     * Attempts to send size_t bytes from buf.
     * @throws net_interface::exception on eof, reset connection, or any other error.
     */
    virtual void send(void* buf, size_t size) = 0;
    
    /**
     * Attempts to receive size_t bytes into buf.
     * @throws net_interface::exception on eof, reset connection, or any other error.
     */    
    virtual void receive(void* buf, size_t size) = 0;

    enum class error_code {
        eof,
        reset,
        other
    };

    class error : public std::runtime_error {
    public:        
        error(char const* what_arg, error_code code)
        : std::runtime_error(what_arg), ec_(code) {}

        error(std::string const& what_arg, error_code code)
        : std::runtime_error(what_arg), ec_(code) {}

        error_code code() const { return ec_; }

    private:
        error_code ec_;
    };
};

#endif // NET_INTERFACE_H
