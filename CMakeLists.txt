cmake_minimum_required(VERSION 3.10)
project(System_Programming_Project)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

add_executable(System_Programming_Project
        Makefile main.c)
