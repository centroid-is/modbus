add_library(modbus
            STATIC
            src/client.cpp
            src/error.cpp)
target_include_directories(modbus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/modbus)