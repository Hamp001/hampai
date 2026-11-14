cmake_minimum_required(VERSION 3.20)

project(MicroAI LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "MicroAI system monitor currently supports Windows only.")
endif()

add_executable(microai
    src/main.cpp
)

target_compile_features(microai PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(microai PRIVATE /W4 /permissive-)
else()
    target_compile_options(microai PRIVATE -Wall -Wextra -Wpedantic)
endif()
