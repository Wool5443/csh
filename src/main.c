#include <stdio.h>

#include "ScratchBuf.h"
#include "cshell.h"

int main(int argc, const char* argv[])
{
    ERROR_CHECKING();

    logger_init_console();

    scratch_init(1024);

    if (argc < 2)
    {
        printf("Usage: %s <path to script file> args...\n", argv[0]);
        ERROR_LEAVE();
    }

    CHECK_ERROR(compile_and_run_file(argv[1], argc - 2, argv + 2));

ERROR_CASE
    return err;
}
