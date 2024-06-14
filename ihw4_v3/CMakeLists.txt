cmake_minimum_required(VERSION 3.26)
project(ihw4_v3 C)

set(CMAKE_C_STANDARD 11)

add_executable(ihw4_v3 main.c
        client.c
        monitor.c
        controller.c)
