cmake_minimum_required(VERSION 2.6)

# treat link directories relative to CMAKE_CURRENT_SOURCE_DIR
cmake_policy(VERSION 2.6) 
link_directories(${CMAKE_SOURCE_DIR}/lib)

add_subdirectory(src)
