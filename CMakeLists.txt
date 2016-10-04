cmake_minimum_required(VERSION 2.6)

project(COMP7005-Assn1)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# treat link directories relative to CMAKE_CURRENT_SOURCE_DIR
cmake_policy(VERSION 2.6) 
link_directories(${CMAKE_SOURCE_DIR}/lib)

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
endif()

add_subdirectory(src)
