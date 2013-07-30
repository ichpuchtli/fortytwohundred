cmake_minimum_required (VERSION 2.8.7)

if(WIN32)
  message(FATAL_ERROR "Sorry, Windows is not supported")
endif()

message("OS PATH: $ENV{PATH}")
message("CMake Build Dir: ${CMAKE_BINARY_DIR}")
message("CMake Source Dir: ${CMAKE_CURRENT_SOURCE_DIR}")

# Google Testing Framework
##############################################################################

enable_testing()

set(gtest_dir gtest)

add_subdirectory(${gtest_dir})

include_directories(${gtest_dir}/include)

set_source_files_properties(tests/integrity_test.c PROPERTIES LANGUAGE CXX)

add_executable(integrity_test tests/integrity_test.c)

target_link_libraries(integrity_test gtest gtest_main)

add_test(IntegrityTest integrity_test)

# Main Program
##############################################################################

add_definitions(-Wall -pedantic)

add_executable(main main.c)

