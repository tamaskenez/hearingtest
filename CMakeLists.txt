cmake_minimum_required(VERSION 3.29)
project(hearingtest)

if(NOT CMAKE_PREFIX_PATH)
	set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/id)
endif()

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG 1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(BUILD_TESTING)
	enable_testing()
	find_package(GTest REQUIRED CONFIG)
endif()

find_package(fmt REQUIRED)
find_package(absl REQUIRED)
find_package(gtest REQUIRED)
find_package(JUCE REQUIRED)

include(cmake/warnings_clang.cmake)
include(cmake/warnings_gcc.cmake)
include(cmake/warnings_msvc.cmake)

add_compile_definitions(DATA_DIR=\"${CMAKE_CURRENT_LIST_DIR}/data\")
add_subdirectory(src)
