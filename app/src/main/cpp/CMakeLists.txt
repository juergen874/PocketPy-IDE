cmake_minimum_required(VERSION 3.22.1)
project("pocketpy_engine")

if(ANDROID)
    add_library(pocketpy_engine SHARED
        pocketpy.c
        pocketpy_jni.c
    )
    target_include_directories(pocketpy_engine PRIVATE .)
    target_link_libraries(pocketpy_engine log m)
else()
    add_executable(pocketpy
        pocketpy.c
        cli_main.c
    )
    target_include_directories(pocketpy PRIVATE .)
    if(WIN32)
        target_link_libraries(pocketpy PRIVATE ws2_32 advapi32)
    else()
        target_link_libraries(pocketpy PRIVATE m)
    endif()
endif()
