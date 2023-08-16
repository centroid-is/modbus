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

option(BUILD_EXAMPLES "Indicates whether examples should be built." ON)
add_feature_info("BUILD_EXAMPLES" BUILD_EXAMPLES "Indicates whether examples should be built.")

add_library(modbus
  src/error.cpp)

target_link_libraries(modbus PRIVATE Boost::boost)
target_include_directories(modbus PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(BUILD_TESTS)
  add_subdirectory(tests)
endif()
