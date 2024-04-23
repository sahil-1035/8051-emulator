cmake_minimum_required(VERSION 3.28)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_COLOR_DIAGNOSTICS ON)

project(8051)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(8051
	main.c
	interface.c
	emulator.c
	definitions.c
	window.c
	utils/set.c)

target_compile_options(8051 PRIVATE
	-Wall -Wextra
)

target_link_libraries(8051 ${CURSES_LIBRARIES})
