cmake_minimum_required(VERSION 3.22.1)
project("pocketpy_engine")

add_library(pocketpy_engine SHARED
    pocketpy.c
    pocketpy_jni.c
)

target_include_directories(pocketpy_engine PRIVATE .)
target_link_libraries(pocketpy_engine log m)
