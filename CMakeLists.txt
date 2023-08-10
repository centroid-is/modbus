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
  STATIC
  src/client.cpp
  src/error.cpp
  src/server.cpp)

target_link_libraries(modbus PRIVATE Boost::boost)
target_include_directories(modbus PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

add_executable(modbus_test src/test.cpp)
target_link_libraries(modbus_test PRIVATE modbus Boost::boost)

add_executable(modbus_server_test src/server_test.cpp)
target_link_libraries(modbus_server_test PRIVATE modbus Boost::boost)

if(BUILD_TESTS)
  add_subdirectory(tests)
endif()
