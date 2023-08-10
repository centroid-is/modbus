enable_testing()

find_package(ut CONFIG REQUIRED)
add_executable(serialization serialization.cpp)

target_link_libraries(serialization PRIVATE Boost::ut modbus)

add_test(NAME serialization COMMAND serialization)