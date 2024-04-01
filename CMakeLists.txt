cmake_minimum_required(VERSION 3.28)

set(CMAKE_BUILD_TYPE Debug)

project(8051)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(8051
	main.c
	interface.c
	emulator.c
	definitions.c
	window.c)

target_link_libraries(8051 ${CURSES_LIBRARIES})
