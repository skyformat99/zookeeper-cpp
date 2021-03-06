cmake_minimum_required(VERSION 3.5)

file(READ src/zk/config.hpp CONFIG_HPP_STR)
string(REGEX REPLACE ".*# *define +ZKPP_VERSION_MAJOR +([0-9]+).*" "\\1" ZKPP_VERSION_MAJOR "${CONFIG_HPP_STR}")
string(REGEX REPLACE ".*# *define +ZKPP_VERSION_MINOR +([0-9]+).*" "\\1" ZKPP_VERSION_MINOR "${CONFIG_HPP_STR}")
string(REGEX REPLACE ".*# *define +ZKPP_VERSION_PATCH +([0-9]+).*" "\\1" ZKPP_VERSION_PATCH "${CONFIG_HPP_STR}")

set(ZKPP_VERSION "${ZKPP_VERSION_MAJOR}.${ZKPP_VERSION_MINOR}.${ZKPP_VERSION_PATCH}")
project(zookeeper-cpp
        LANGUAGES CXX
        VERSION "${ZKPP_VERSION}"
       )
set(PROJECT_SO_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
message(STATUS "Software Version: ${ZKPP_VERSION}")

################################################################################
# CMake                                                                        #
################################################################################

cmake_policy(VERSION 3.5)
cmake_policy(SET CMP0037 OLD) # allow generation of "test" target

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")

include(BuildFunctions)
include(CheckIncludeFileCXX)
include(ConfigurationSetting)
include(ListSplit)
include(ZooKeeper)

################################################################################
# Build Configuration                                                          #
################################################################################

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
  message(STATUS "No build type selected, default to ${CMAKE_BUILD_TYPE}")
endif()

set(VALID_BUILD_TYPES Debug Release)
if(NOT ${CMAKE_BUILD_TYPE} IN_LIST VALID_BUILD_TYPES)
  message(FATAL_ERROR "Invalid CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}\nValid build types are: ${VALID_BUILD_TYPES}")
endif()
message(STATUS "Configuration: ${CMAKE_BUILD_TYPE}")

set(ZKPP_SERVER_VERSIONS "3.5.3-beta;3.4.10"
    CACHE STRING "The ZooKeeper server versions to run tests against. The first in the list is the default."
   )

message(STATUS "Features:")
build_option(NAME       CODE_COVERAGE
             DOC        "Enable code coverage (turns on the test-coverage target)"
             DEFAULT    OFF
             CONFIGS_ON Debug
            )

configuration_setting(NAME    BUFFER
                      DOC     "Type to use for zk::buffer"
                      DEFAULT STD_VECTOR
                      OPTIONS
                        STD_VECTOR
                        CUSTOM
                     )

configuration_setting(NAME    FUTURE
                      DOC     "Type to use for zk::future<T> and zk::promise<T>"
                      DEFAULT STD
                      OPTIONS
                        STD
                        STD_EXPERIMENTAL
                        CUSTOM
                     )

check_include_file_cxx("optional" HAVE_STD_OPTIONAL)
if(HAVE_STD_OPTIONAL)
  set(OPTIONAL_DEFAULT STD)
else()
  set(OPTIONAL_DEFAULT STD_EXPERIMENTAL)
endif()
configuration_setting(NAME    OPTIONAL
                      DOC     "Type to use for zk::optional<T>, zk::nullopt_t, and zk::nullopt"
                      DEFAULT STD
                      SET     ${OPTIONAL_DEFAULT}
                      OPTIONS
                        STD
                        STD_EXPERIMENTAL
                        CUSTOM
                     )

check_include_file_cxx("string_view" HAVE_STD_STRING_VIEW)
if(HAVE_STD_STRING_VIEW)
  set(STRING_VIEW_DEFAULT STD)
else()
  set(STRING_VIEW_DEFAULT STD_EXPERIMENTAL)
endif()
configuration_setting(NAME    STRING_VIEW
                      DOC     "Type to use for zk::string_view"
                      DEFAULT STD
                      SET     ${STRING_VIEW_DEFAULT}
                      OPTIONS
                        STD
                        STD_EXPERIMENTAL
                        CUSTOM
                     )

set(CXX_STANDARD c++17
    CACHE STRING "The language standard to target for C++."
   )

set(CXX_WARNINGS "-Wall -Wextra -Wconversion -Werror")
set(CXX_EXTRA_FLAGS "-Wl,--no-as-needed")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=${CXX_STANDARD} ${CXX_WARNINGS} -ggdb3 ${CXX_EXTRA_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -DZKPP_DEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

################################################################################
# External Libraries                                                           #
################################################################################

find_library(zookeeper_LIBRARIES zookeeper_mt)

include_directories("${PROJECT_SOURCE_DIR}/src")

################################################################################
# GTest                                                                        #
################################################################################

find_package(GTest)

if(GTest_FOUND)
  # Unclear why GTest packaging behaves differently than every other find_package
  set(gtest_LIBRARIES GTest::GTest)
elseif(EXISTS "/usr/src/gtest/src/")
  # GTest's packaging on Ubuntu (libgtest-dev) contains all the source files instead of a library and headers. CMake has
  # a package discovery for GTest, but it does not pick up on this for you, so we'll build it manually here.
  message(STATUS "GTest is not installed, but the sources were found...adding them to the build")
  if(EXISTS "/usr/src/gtest/src/gtest-all.cc")
    set(gtest_lib_cpps "/usr/src/gtest/src/gtest-all.cc")
    message(STATUS "Building with gtest-all.cc")
  else()
    file(GLOB gtest_lib_cpps "/usr/src/gtest/src/gtest-*.cc")
    message(STATUS "Building with gtest_lib_cpps=${gtest_lib_cpps}")
  endif()
  add_library(gtest SHARED ${gtest_lib_cpps})

  # GTest uses relative imports incorrectly, so make sure to add it to the include path.
  target_include_directories(gtest PRIVATE "/usr/src/gtest")
  # Also disable -Werror
  target_compile_options(gtest PRIVATE "-Wno-error")

  set(gtest_LIBRARIES gtest)
else()
  message(SEND_ERROR "GTest was not found")
endif()

################################################################################
# Building                                                                     #
################################################################################

build_module(NAME zkpp_tests
             PATH src/zk/tests
             LINK_LIBRARIES
               ${gtest_LIBRARIES}
            )

build_module(NAME zkpp
             PATH src/zk
             NO_RECURSE
             LINK_LIBRARIES
               ${zookeeper_LIBRARIES}
            )

build_module(NAME zkpp_server
             PATH src/zk/server
             LINK_LIBRARIES
               zkpp
            )

target_link_libraries(zkpp__tests zkpp_server zkpp_server__tests)

################################################################################
# ZooKeeper Server Testing                                                     #
################################################################################

foreach(server_version IN LISTS ZKPP_SERVER_VERSIONS)
  find_zookeeper_server(VERSION "${server_version}"
                        OUTPUT_CLASSPATH server_classpath
                       )
  set(generated_cpp "${CMAKE_CURRENT_BINARY_DIR}/generated/src/zk/server/classpath_registration_${server_version}.cpp")
  configure_file(src/zk/server/classpath_registration_template.cpp.in "${generated_cpp}" @ONLY)
  target_sources(zkpp_server__tests PRIVATE "${generated_cpp}")
endforeach()

################################################################################
# Targets                                                                      #
################################################################################

add_custom_target(test
                  COMMAND $<TARGET_FILE:zkpp_tests_prog>
                          "--gtest_output=xml:test-results.xml"
                          "--gtest_death_test_style=threadsafe"
                  DEPENDS zkpp_tests_prog
                  BYPRODUCTS test-results.xml
                  USES_TERMINAL
                 )

# Similar to test, but run it inside of GDB with GTest options one would want when running in GDB.
add_custom_target(gdbtest
                  COMMAND "gdb"
                          "-args"
                          $<TARGET_FILE:zkpp_tests_prog>
                          "--gtest_output=xml:test-results-gdb.xml"
                          "--gtest_death_test_style=threadsafe"
                          "--gtest_break_on_failure=1"
                          "--gtest_catch_exceptions=0"
                  DEPENDS zkpp_tests_prog
                  BYPRODUCTS test-results-gdb.xml
                  USES_TERMINAL
                 )

if(ZKPP_BUILD_OPTION_CODE_COVERAGE)
  include(CodeCoverage)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  setup_target_for_coverage(test-coverage zkpp_tests_prog coverage "--gtest_death_test_style=threadsafe")
endif()
