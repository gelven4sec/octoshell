cmake_minimum_required(VERSION 3.17)
project(octoshell C)

set(CMAKE_C_STANDARD 99)

add_executable(octoshell shell.c main.c)