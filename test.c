#!/usr/bin/env csh

//INCLUDE "-I./include" "-I./cmlib/Vector" "-I./cmlib/Logger"
//LINK "-lm"
//FILES "aboba.c"
//FILES "cmlib/Logger/src/Logger.c"
//FILES "cmlib/Vector/src/Vector.c"
//FLAGS "-fsanitize=address"

#include <stdio.h>
#include "Vector.h"
#include "aboba.h" // aboba.c contains void foo(void) which prints foo on the screen

int main(int argc, const char* argv[])
{
    foo();

    if (argc == 1) return 0;

    printf("%s\n", argv[1]);

    int* numbers = {};

    vec_add(numbers, 10);

    printf("%d\n", numbers[0]);

    vec_dtor(numbers);

    return 0;
}
