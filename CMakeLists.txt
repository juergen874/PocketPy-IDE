cmake_minimum_required(VERSION 3.20)
project(PocketPy LANGUAGES C)

set(CMAKE_C_STANDARD 11)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES _WINSOCK_DEPRECATED_NO_WARNINGS)
    add_compile_options(/W3 /utf-8 /O2)
else()
    add_compile_options(-O2 -Wall)
endif()

add_executable(pocketpy
    app/src/main/cpp/pocketpy.c
    app/src/main/cpp/cli_main.c
)

target_include_directories(pocketpy PRIVATE app/src/main/cpp)

if(WIN32)
    target_link_libraries(pocketpy PRIVATE ws2_32 advapi32)
else()
    target_link_libraries(pocketpy PRIVATE m)
endif()

install(TARGETS pocketpy DESTINATION bin)
