enable_testing()

find_package(ut CONFIG REQUIRED)

add_executable(serialization serialization.cpp)
target_link_libraries(serialization PRIVATE Boost::ut modbus)
add_test(NAME serialization COMMAND serialization)

add_executable(deserialization deserialization.cpp)
target_link_libraries(deserialization PRIVATE Boost::ut modbus)
add_test(NAME deserialization COMMAND deserialization)

add_executable(sniff_request_encoding helpers/mbpoll_request_encoding_sniffer.cpp)
target_link_libraries(sniff_request_encoding PRIVATE Boost::boost modbus)