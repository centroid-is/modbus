cmake_minimum_required(VERSION 3.22)
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

# find_package(PkgConfig REQUIRED)
# pkg_check_modules(asio REQUIRED IMPORTED_TARGET GLOBAL asio)

set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
include(CTest)

include(FeatureSummary)

option(BUILD_TESTS "Indicates whether tests should be built." OFF)
add_feature_info("BUILD_TESTS" BUILD_TESTS "Indicates whether tests should be built.")

option(BUILD_EXAMPLES "Indicates whether examples should be built." OFF)
add_feature_info("BUILD_EXAMPLES" BUILD_EXAMPLES "Indicates whether examples should be built.")

add_library(modbus
  src/error.cpp)

# Locate standalone asio. We require >= 1.22 (for as_tuple); Ubuntu jammy
# ships 1.18, so we prefer a vendored copy under /opt/asio.
find_path(ASIO_INCLUDE_DIR
  NAMES asio/as_tuple.hpp
  HINTS /opt/asio/include
  REQUIRED)

target_include_directories(modbus PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${ASIO_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
        $<INSTALL_INTERFACE:${ASIO_INCLUDE_DIR}>
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

# --- CPack: produce a binary .deb -------------------------------------------
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "recompute-modbus")
set(CPACK_PACKAGE_VENDOR "Centroid")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
  "Boost.Asio-based Modbus library (header-only deps; static link)")
set(CPACK_PACKAGE_CONTACT "Centroid <build@centroid.local>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "libdevel")
# Depends on the vendored asio (>= 1.22) shipped as recompute-vendor-headers.
set(CPACK_DEBIAN_PACKAGE_DEPENDS "recompute-vendor-headers")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
include(CPack)

