enable_testing()

find_package(ut CONFIG REQUIRED)

# Binary encoding tests
add_executable(serialization serialization.cpp)
target_link_libraries(serialization PRIVATE Boost::ut modbus)
add_test(NAME serialization COMMAND serialization)

add_executable(deserialization deserialization.cpp)
target_link_libraries(deserialization PRIVATE Boost::ut modbus)
add_test(NAME deserialization COMMAND deserialization)

add_executable(integration integration.cpp)
target_link_libraries(integration PRIVATE Boost::ut modbus)
add_test(NAME integration COMMAND integration)

# Modbus application conformance tests
add_executable(server_conformance conformance/server.cpp)
target_link_libraries(server_conformance PRIVATE Boost::ut modbus)
add_test(NAME server_conformance COMMAND server_conformance)

add_executable(sniff_request_encoding helpers/mbpoll_request_encoding_sniffer.cpp)
target_link_libraries(sniff_request_encoding PRIVATE Boost::boost modbus)