#   CMake assembly file for libmatfile project.

cmake_minimum_required(VERSION 3.3)
cmake_policy(SET CMP0011 NEW)
cmake_policy(SET CMP0048 NEW)

project(matfile
        VERSION 0.1.0
        LANGUAGES C CXX)

option(DOXYGEN_HTML "Build HTML documentation with Doxygen." ON)

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

#   Import targets from dependencies.
add_subdirectory(deps/googletest EXCLUDE_FROM_ALL)

#   Common compiler options.
set(CMAKE_C_STANDARD 11)

#   Set up search paths.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googletest/include)

#   Define sources and source groups.
set(LIB_SOURCES src/matfile.c
                src/tape.c)
set(CLI_SOURCES src/main.cc)
set(TEST_SOURCES test/main.cc
                 test/reader.cc
                 test/writer.cc)

source_group(lib-sources FILES ${LIB_SOURCES})
source_group(test-sources FILES ${TEST_SOURCES})

#   Build executable and library.
add_library(matfile-obj OBJECT ${LIB_SOURCES})
add_library(matfile-static STATIC $<TARGET_OBJECTS:matfile-obj>)
add_library(matfile-shared SHARED $<TARGET_OBJECTS:matfile-obj>)
add_executable(matfile-cli $<TARGET_OBJECTS:matfile-obj> ${CLI_SOURCES})

set_property(TARGET matfile-obj PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET matfile-static PROPERTY OUTPUT_NAME matfile)
set_property(TARGET matfile-shared PROPERTY OUTPUT_NAME matfile)

target_link_libraries(matfile-static ${ZLIB_LIBRARIES})
target_link_libraries(matfile-shared ${ZLIB_LIBRARIES})
target_link_libraries(matfile-cli ${ZLIB_LIBRARIES})

#   Define test to run.
enable_testing()

if(BUILD_TESTING)
    add_executable(matfile-test $<TARGET_OBJECTS:matfile-obj> ${TEST_SOURCES})
    add_test(NAME test-all COMMAND matfile-test)
    target_link_libraries(matfile-test ${ZLIB_LIBRARIES} gtest_main)
endif(BUILD_TESTING)

#   Install executables and libs.
install(TARGETS matfile-cli matfile-shared matfile-static
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(DIRECTORY include/matfile/
        DESTINATION include/matfile
        FILES_MATCHING PATTERN "*.h")

install(FILES LICENSE
        DESTINATION share/licenses/matfile)

#   Configured documentation tools and intermediate build results.
if(DOXYGEN_HTML AND DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doc")
    set(DOXYGEN_XML_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doc/xml")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile"
                   @ONLY)

    add_custom_target(matfile-doxygen-docs ALL
                      "${DOXYGEN_EXECUTABLE}"
                      "${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile" > /dev/null
                      COMMENT "Building documentation with Doxygen")
elseif(NOT DOXYGEN_FOUND)
    message(STATUS
            "WARNING: Doxygen not found. Skip doxygen doc building")
else(DOXYGEN_HTML AND DOXYGEN_FOUND)
    message(STATUS
            "NOTE: Skip doxygen doc building")
endif(DOXYGEN_HTML AND DOXYGEN_FOUND)
