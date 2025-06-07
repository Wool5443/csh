cmake_minimum_required(VERSION 3.15)
project("cshell" C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED true)

# set(CMAKE_C_FLAGS "-O0 -Wall -Wextra -fsanitize=address -g")
set(CMAKE_BUILD_TYPE Release)

add_subdirectory(cmlib)

set(SOURCE
    "src/main.c"
    "src/cshell.c"
    "src/Gather_args.c"
    "src/Compiler_args_list.c"
)

add_executable(${PROJECT_NAME} ${SOURCE})
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE cmlib_logger cmlib_string cmlib_vector cmlib_IO cmlib_scratch_buffer)
target_compile_definitions(${PROJECT_NAME} PRIVATE DISABLE_LOGGING)
