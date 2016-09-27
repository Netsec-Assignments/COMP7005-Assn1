include_directories(${CMAKE_SOURCE_DIR}/include)
cmake_minimum_required(VERSION 2.6)

set(SOURCES main.cpp
            server.cpp)

add_executable(server ${SOURCES})
target_link_libraries(server boost_filesystem boost_system pthread)
