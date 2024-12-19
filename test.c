#!/usr/bin/csh

#include <stdio.h>

int main(int argc, const char* argv[])
{
    if (argc == 1) return 0;

    printf("%s\n", argv[1]);

    return 0;
}
