cmake_minimum_required(VERSION 3.15)
set(projectName "cshell")
project(${projectName} C)

set(CMAKE_C_FLAGS "-O0 -Wall -Wextra -fsanitize=address -g")

add_subdirectory(cmlib)

set(SOURCE
    "src/main.c"
    "src/cshell.c"
)

add_executable(${projectName} ${SOURCE})
target_include_directories(${projectName} PRIVATE include)
target_link_libraries(${projectName} PRIVATE cmlibLogger cmlibString cmlibVector cmlibIOUtils cmlibScratchBuffer)
# target_compile_definitions(${projectName} PUBLIC DISABLE_LOGGING)
