cmake_minimum_required(VERSION 3.15)
set(projectName "cshell")
project(${projectName} C)

add_subdirectory(cmlib)

set(SOURCE
    "src/main.c"
)

add_executable(${projectName} ${SOURCE})
target_include_directories(${projectName} PRIVATE include)
target_link_libraries(${projectName} PRIVATE cmlibLogger cmlibString cmlibScratchBuffer)
