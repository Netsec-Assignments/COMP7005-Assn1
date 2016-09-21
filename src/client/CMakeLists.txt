include_directories(${CMAKE_SOURCE_DIR}/include)
set(SOURCES main.cpp
            client.cpp)

add_executable(client ${SOURCES})
target_link_libraries(client boost_filesystem boost_system pthread)
