cmake_minimum_required(VERSION 3.15)
project("cshell" C)

# set(CMAKE_C_FLAGS "-O0 -Wall -Wextra -fsanitize=address -g")
set(CMAKE_BUILD_TYPE Release)

add_subdirectory(cmlib)

set(SOURCE
    "src/main.c"
    "src/cshell.c"
    "src/GatherArgs.c"
    "src/CompilerArgsList.c"
)

add_executable(${PROJECT_NAME} ${SOURCE})
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE cmlibLogger cmlibString cmlibVector cmlibIO cmlibScratchBuffer)
target_compile_definitions(${PROJECT_NAME} PRIVATE DISABLE_LOGGING)
