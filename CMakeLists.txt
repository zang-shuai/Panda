cmake_minimum_required(VERSION 3.28)
project(Panda C)

set(CMAKE_C_STANDARD 11)

add_executable(Panda main.c
        chunk.c
        memory.c
        debug.c
        debug.h
        value.c
        value.h
        vm.c
        vm.h
        compiler.c
        compiler.h
        scanner.c
        scanner.h
        object.c
        object.h)
