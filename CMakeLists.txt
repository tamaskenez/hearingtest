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

find_package(fmt REQUIRED CONFIG)
find_package(absl REQUIRED CONFIG)
find_package(gtest REQUIRED CONFIG)
find_package(JUCE REQUIRED CONFIG)

unset(Curses_DIR CACHE)
if(false)
	find_package(Curses REQUIRED CONFIG)
else()
	set(CURSES_NEED_NCURSES 1)
	find_package(Curses REQUIRED)
	add_library(Curses INTERFACE)
	target_include_directories(Curses INTERFACE ${CURSES_INCLUDE_DIRS})
	target_link_libraries(Curses INTERFACE ${CURSES_LIBRARIES})
	target_compile_definitions(Curses INTERFACE ${CURSES_CFLAGS})
	add_library(Curses::Curses ALIAS Curses)
endif()

add_library(jucemodules OBJECT)

target_link_libraries(jucemodules PRIVATE
	juce::juce_audio_formats
	juce::juce_audio_devices
	juce::juce_audio_utils
	juce::juce_events
)

target_compile_definitions(jucemodules
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        DONT_SET_USING_JUCE_NAMESPACE=1
        JUCE_MODAL_LOOPS_PERMITTED=1
    INTERFACE
		$<TARGET_PROPERTY:jucemodules,COMPILE_DEFINITIONS>
)

target_include_directories(jucemodules
    INTERFACE
        $<TARGET_PROPERTY:jucemodules,INCLUDE_DIRECTORIES>
)

add_compile_definitions(DATA_DIR=\"${CMAKE_CURRENT_LIST_DIR}/data\")
add_subdirectory(src)
