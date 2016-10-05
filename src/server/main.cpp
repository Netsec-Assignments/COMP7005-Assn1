/* ========================================================================
   $File: main.cpp $
   $Program: $
   $Developer: Mat Siwoski/Shane Spoor $
   $Created On: 2016/09/23 $
   $Description: $
   $Revisions: $
   ======================================================================== */

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <server/server.h>


/* ========================================================================
   $ FUNCTION
   $ Name: main $
   $ Prototype: int main(int argc, char** argv) { $
   $ Params: 
   $    argc: the number of arguments
   $    argv: the argument received
   $ Description:  Starts the server
   ======================================================================== */
int main(int argc, char** argv) {
    if(argc != 2) {
        std::cout << "usage: " << argv[0] << " [storage directory]" << std::endl;
        return 1;
    }

    boost::asio::io_service service;
    
    try {
        std::string p(argv[1]);
        // Might figure out how to thread this later, but for now just Ctrl + C out of it
        server s(service, p);
        s.start();
    } catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
