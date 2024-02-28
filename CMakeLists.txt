cmake_minimum_required(VERSION 3.27)
project(modbus
  VERSION
    0.0.1
  DESCRIPTION
    "Boost asio based modbus library"
  LANGUAGES
    CXX
)

add_compile_definitions(BOOST_UT_DISABLE_MODULE)

set(CMAKE_CXX_STANDARD 23)

find_package(asio REQUIRED)

set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
include(CTest)

include(FeatureSummary)

option(BUILD_TESTS "Indicates whether tests should be built." ON)
add_feature_info("BUILD_TESTS" BUILD_TESTS "Indicates whether tests should be built.")

option(BUILD_EXAMPLES "Indicates whether examples should be built." ON)
add_feature_info("BUILD_EXAMPLES" BUILD_EXAMPLES "Indicates whether examples should be built.")

add_library(modbus
  src/error.cpp)

target_link_libraries(modbus PUBLIC asio)
target_include_directories(modbus PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# target_include_directories(ipc
#         PUBLIC
#         $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc/public>
#         $<INSTALL_INTERFACE:include>
#         $<BUILD_INTERFACE:${AZMQ_INCLUDE_DIRS}>
# )

include(GNUInstallDirs)

install(TARGETS modbus
        EXPORT modbus
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)

if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(BUILD_TESTS)
  add_subdirectory(tests)
endif()

# Clang format all files
file(GLOB_RECURSE ALL_SOURCE_FILES src/*.cpp include/**/*.hpp examples/*.cpp tests/*.cpp)
add_custom_target(
        clangformat-fix
        COMMAND clang-format
        --style=file
        -i
        ${ALL_SOURCE_FILES}
)
add_custom_target(
        clangformat-dry
        COMMAND clang-format
        --style=file
        -i
        --dry-run
        ${ALL_SOURCE_FILES}
)
