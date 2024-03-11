cmake_minimum_required(VERSION 3.28)

set(CMAKE_BUILD_TYPE Debug)

project(8051)

add_executable(8051
	main.c
	interface.c
	emulator.c
	definitions.c)
