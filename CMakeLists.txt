cmake_minimum_required(VERSION 3.23)

add_library(rswf STATIC
    flashpck.c
    flashpck.h
)

target_include_directories(rswf
    PUBLIC
        ./
)