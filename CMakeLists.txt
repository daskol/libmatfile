#   CMake assembly file for libmatfile project.

cmake_minimum_required(VERSION 3.3)
cmake_policy(SET CMP0011 NEW)
cmake_policy(SET CMP0048 NEW)

project(matfile
        VERSION 0.0.0
        LANGUAGES C CXX)

#   Make sure that the default is a RELEASE.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RELEASE CACHE STRING
        "Choose the type of build, options are: None Debug Release."
        FORCE)
endif(NOT CMAKE_BUILD_TYPE)

#   Find packages.
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Sphinx REQUIRED)
find_package(Doxygen REQUIRED)
find_package(ZLIB REQUIRED)

message(STATUS "${ZLIB_LIBRARIES}")
message(STATUS "${ZLIB_INCLUDE_DIRS}")

#   Common compiler options.
set(CMAKE_C_STANDARD 11)

#   Set up search paths.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#   Define sources and source groups.
set(LIB_SOURCES src/matfile.c
                src/tape.c)
set(CLI_SOURCES src/main.cc)

source_group(lib-sources FILES ${LIB_SOURCES})

#   Build executable and library.
add_library(matfile-obj OBJECT ${LIB_SOURCES})
add_library(matfile STATIC $<TARGET_OBJECTS:matfile-obj>)
add_executable(matfile-cli $<TARGET_OBJECTS:matfile-obj> ${CLI_SOURCES})

target_link_libraries(matfile ${ZLIB_LIBRARIES})
target_link_libraries(matfile-cli ${ZLIB_LIBRARIES})

#   Define test to run.
enable_testing()

#   TODO: Install executables and libs.

#   Configured documentation tools and intermediate build results.

if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doc")
    set(DOXYGEN_XML_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doc/xml")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile"
                   @ONLY)

    add_custom_target(matfile-doxygen-docs ALL
                      "${DOXYGEN_EXECUTABLE}"
                      "${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile" > /dev/null
                      COMMENT "Building documentation with Doxygen")
else(DOXYGEN_FOUND)
    message(STATUS
            "WARNING: Doxygen not found. Skip doxygen doc building")
endif(DOXYGEN_FOUND)
