#include <stdio.h>

#include "cshell.h"

int main(int argc, const char* argv[])
{
    ERROR_CHECKING();

    LoggerInitConsole();

    if (argc < 2)
    {
        printf("Usage: %s <path to script file> args...\n", argv[0]);
        ERROR_LEAVE();
    }

    LoggerFinish();

    CHECK_ERROR(CompileAndRunFile(argv[1], argv + 2, argv[0]));

ERROR_CASE

    LoggerFinish();
    return err;
}
