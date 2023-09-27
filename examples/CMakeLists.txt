
add_executable(client_example client_example.cpp)
target_link_libraries(client_example PRIVATE modbus Boost::boost)

add_executable(server_example server_example.cpp)
target_link_libraries(server_example PRIVATE modbus Boost::boost)