cmake_minimum_required(VERSION 3.20.4)

find_package(Zephyr REQUIRED HINTS ../zephyr)
project("uart-peripheral-driver-example")

target_sources(app PRIVATE
    src/main.c
)
add_subdirectory(drivers)