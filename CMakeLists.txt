project(MODBUS VERSION 0.0.0)
add_library(modbus
            STATIC
            src/client.cpp
            src/error.cpp
            src/server.cpp)
target_include_directories(modbus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)

add_executable(modbus_test src/test.cpp)
target_link_libraries(modbus_test PRIVATE modbus pthread)

add_executable(modbus_server_test src/server_test.cpp)
target_link_libraries(modbus_server_test PRIVATE modbus pthread)
