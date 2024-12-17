cmake_minimum_required(VERSION 3.25)
project(modbus
  VERSION
    2024.4.1
  DESCRIPTION
    "Boost asio based modbus library"
  LANGUAGES
    CXX
)

add_compile_definitions(BOOST_UT_DISABLE_MODULE)

set(CMAKE_CXX_STANDARD 23)


set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
include(CTest)

include(FeatureSummary)
find_package(PkgConfig REQUIRED)

pkg_check_modules(asio REQUIRED IMPORTED_TARGET GLOBAL asio)

option(BUILD_TESTS "Indicates whether tests should be built." ON)
add_feature_info("BUILD_TESTS" BUILD_TESTS "Indicates whether tests should be built.")

option(BUILD_EXAMPLES "Indicates whether examples should be built." ON)
add_feature_info("BUILD_EXAMPLES" BUILD_EXAMPLES "Indicates whether examples should be built.")

add_library(modbus
  src/error.cpp)

target_link_libraries(modbus PUBLIC PkgConfig::asio)
target_include_directories(modbus PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

include(GNUInstallDirs)

install(TARGETS modbus
        EXPORT modbusTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)


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



# Let's go ahead and install everything properly
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(modbus_PACKAGE_NAME modbus)
set(modbus_INSTALL_CMAKEDIR ${CMAKE_INSTALL_DATADIR}/${modbus_PACKAGE_NAME})

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${modbus_PACKAGE_NAME}ConfigVersion.cmake"
        VERSION ${CMAKE_PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        "${CMAKE_CURRENT_LIST_DIR}/${modbus_PACKAGE_NAME}Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${modbus_PACKAGE_NAME}Config.cmake"
        INSTALL_DESTINATION ${modbus_INSTALL_CMAKEDIR}
)

install(EXPORT ${modbus_PACKAGE_NAME}Targets DESTINATION ${modbus_INSTALL_CMAKEDIR})
install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${modbus_PACKAGE_NAME}ConfigVersion.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${modbus_PACKAGE_NAME}Config.cmake"
        DESTINATION ${modbus_INSTALL_CMAKEDIR}
)

install(
        EXPORT ${modbus_PACKAGE_NAME}Targets
        NAMESPACE modbus::
        DESTINATION ${modbus_INSTALL_CMAKEDIR}
)

