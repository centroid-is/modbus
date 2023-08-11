cmake_minimum_required(VERSION 3.25)
project(modbus
  VERSION
    0.0.1
  DESCRIPTION
    "Boost asio based modbus library"
  LANGUAGES
    CXX
)

set(CMAKE_CXX_STANDARD 23)

find_package(Boost REQUIRED)

include(FeatureSummary)

option(BUILD_TESTS "Indicates whether tests should be built." ON)
add_feature_info("BUILD_TESTS" BUILD_TESTS "Indicates whether tests should be built.")

add_library(modbus
  src/client.cpp
  src/error.cpp)

target_link_libraries(modbus PRIVATE Boost::boost)
target_include_directories(modbus PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

add_executable(client_example examples/client_example.cpp)
target_link_libraries(client_example PRIVATE modbus Boost::boost)

add_executable(server_example examples/server_example.cpp)
target_link_libraries(server_example PRIVATE modbus Boost::boost)
target_compile_definitions(server_example PRIVATE BOOST_ASIO_ENABLE_HANDLER_TRACKING)

if(BUILD_TESTS)
  add_subdirectory(tests)
endif()
