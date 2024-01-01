cmake_minimum_required(VERSION 3.25)
project(kozos C)

set(CMAKE_C_STANDARD 11)

include_directories(src/01/bootload)

add_executable(kozos
        src/01/bootload/defines.h
        src/01/bootload/lib.c
        src/01/bootload/lib.h
        src/01/bootload/main.c
        src/01/bootload/serial.c
        src/01/bootload/serial.h
        src/01/bootload/vector.c
        src/tools/h8write/h8write.c)
