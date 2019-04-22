cmake_minimum_required(VERSION 3.10)
project(DOMP)

set(CMAKE_CXX_STANDARD 11)

add_executable(DOMP
        lib/Makefile
        README.md lib/util/linkedlist.h lib/util/linkedlist.cpp tests/arraySum.cpp lib/domp.h lib/domp.cpp lib/MPIServer.cpp lib/MPIServer.h)
