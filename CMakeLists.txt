cmake_minimum_required(VERSION 3.5)

project(ccpy)

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -Werror")

file(GLOB_RECURSE SRC_LIST src/lccpy/*.cc)

include_directories(
  src
)

add_library(lccpy STATIC ${SRC_LIST})

add_executable(ccpy src/ccpy.cc)
target_link_libraries(ccpy lccpy)

# Tests
set(TEST_LIST "")
file(GLOB_RECURSE TEST_SOURCES tests/*.cc)
foreach(TEST_PATH ${TEST_SOURCES})
  get_filename_component(TEST_NAME ${TEST_PATH} NAME)
  get_filename_component(TEST_NAME_SHORT ${TEST_PATH} NAME_WE)
  string(REGEX REPLACE "\\.[^.]*$" "" TEST_NAME_LONG ${TEST_NAME})
  string(REGEX REPLACE "^[^.]*\\." "" TEST_SPEC_SAMPLE ${TEST_NAME_LONG})

  if(${TEST_SPEC_SAMPLE} STREQUAL ${TEST_NAME_LONG}) # Replace failed
    file(GLOB TEST_CASES RELATIVE "${PROJECT_BINARY_DIR}" "tests/${TEST_NAME_SHORT}/*.in")
  else()
    set(TEST_CASES "../tests/${TEST_NAME_SHORT}/${TEST_SPEC_SAMPLE}.in")
  endif()

  set(TEST_LIST "${TEST_LIST}${TEST_NAME_LONG}\n")
  foreach(CASE ${TEST_CASES})
    set(TEST_LIST "${TEST_LIST}${CASE}\n")
  endforeach()
  set(TEST_LIST "${TEST_LIST}\n")

  add_executable("test_${TEST_NAME_LONG}" ${TEST_PATH})
  target_link_libraries("test_${TEST_NAME_LONG}" lccpy)
endforeach()

file(WRITE "${PROJECT_BINARY_DIR}/test_list.txt" ${TEST_LIST})

add_executable(test src/test.cc)
# target_link_libraries(test lccpy)
