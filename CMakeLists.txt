
cmake_minimum_required(VERSION 3.20.0)

# This project is designed for nRF Connect SDK v3.0.0 or later
# Compatible with nRF Connect SDK 3.0.0+ and Zephyr SDK 0.16.8+
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wm1110-asset-tracker)

FILE(GLOB app_sources
    src/*.c
    src/sidewalk/*.c
    src/lr1110/*.c
    src/peripherals/*.c
)

target_sources(app PRIVATE ${app_sources})

zephyr_include_directories(
    include
)