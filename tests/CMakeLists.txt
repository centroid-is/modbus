enable_testing()

find_package(ut CONFIG REQUIRED)

# Binary encoding tests
add_executable(serialization serialization.cxx)
target_link_libraries(serialization PRIVATE Boost::ut modbus)
add_test(NAME serialization COMMAND serialization)

add_executable(deserialization deserialization.cxx)
target_link_libraries(deserialization PRIVATE Boost::ut modbus)
add_test(NAME deserialization COMMAND deserialization)

add_executable(integration integration.cxx)
target_link_libraries(integration PRIVATE Boost::ut modbus)
add_test(NAME integration COMMAND integration)

add_executable(sniff_request_encoding helpers/mbpoll_request_encoding_sniffer.cpp)
target_link_libraries(sniff_request_encoding PRIVATE modbus)