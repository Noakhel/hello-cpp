cmake_minimum_required(VERSION 3.11)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(hello_cpp C)

set(CMAKE_C_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.0
)
FetchContent_MakeAvailable(raylib)

add_executable(sim main.c graph.c graph.h)
target_link_libraries(sim raylib m)