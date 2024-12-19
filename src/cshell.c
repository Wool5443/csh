#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/wait.h>

#include "ScratchBuf.h"
#include "cshell.h"

static const Str TEMP_FOLDER = { "/tmp/", strlen(TEMP_FOLDER.data) };

ResultString sanitizeDirectoryPath(const char path[static 1]);
const char* getCompiler();

ErrorCode CompileFile(const char path[static 1])
{
    ERROR_CHECKING();

    assert(path);

    String goodPath = {};
    String outputPath = {};

    ResultString goodPathResult = sanitizeDirectoryPath(path);
    CHECK_ERROR(goodPathResult.errorCode);
    goodPath = goodPathResult.value;

    ResultString outputPathResult = StringCtorFromStr(TEMP_FOLDER);
    CHECK_ERROR(outputPathResult.errorCode);
    outputPath = outputPathResult.value;

    CHECK_ERROR(StringAppendString(&outputPath, goodPath));

    char* slash = strchr(outputPath.data + TEMP_FOLDER.size, '\\');
    while (slash)
    {
        *slash = '*';
        slash = strchr(slash + 1, '\\');
    }

    const char* compileArgs[] = {
        getCompiler(),
        goodPath.data,
        "-O3",
        "-o",
        outputPath.data,
        NULL,
    };
    pid_t compile = vfork();

    if (compile == -1)
    {
        HANDLE_ERRNO_ERROR("Error vfork: %s");
    }
    else if (compile == 0)
    {
        execvp(compileArgs[0], (char**)compileArgs);

        HANDLE_ERRNO_ERROR("Error execvp: %s");
    }

    if (wait(NULL) == -1)
    {
        HANDLE_ERRNO_ERROR("Error wait: %s");
    }

    const char* runArgs[] = {
        outputPath.data,
        NULL,
    };
    pid_t run = vfork();

    if (run == -1)
    {
        HANDLE_ERRNO_ERROR("Error vfork: %s");
    }
    else if (run == 0)
    {
        execvp(runArgs[0], (char**)runArgs);

        HANDLE_ERRNO_ERROR("Error execvp: %s");
    }

    if (wait(NULL) == -1)
    {
        HANDLE_ERRNO_ERROR("Error wait: %s");
    }

    const char* deleteArgs[] = {
        "rm",
        outputPath.data,
        NULL,
    };
    pid_t delete = vfork();

    if (delete == -1)
    {
        HANDLE_ERRNO_ERROR("Error vfork: %s");
    }
    else if (delete == 0)
    {
        execvp(deleteArgs[0], (char**)deleteArgs);

        HANDLE_ERRNO_ERROR("Error execvp: %s");
    }

ERROR_CASE
    StringDtor(&goodPath);
    StringDtor(&outputPath);

    return err;
}

ResultString sanitizeDirectoryPath(const char path[static 1])
{
    ERROR_CHECKING();

    assert(path);

    char goodPath[PATH_MAX + 1] = "";

    if (!realpath(path, goodPath))
    {
        HANDLE_ERRNO_ERROR("Error realpath for %s: %s", path);
    }

    size_t size = strlen(goodPath);

    goodPath[size] = '/';
    size++;

    return StringCtorFromStr(StrCtorSize(goodPath, size));

ERROR_CASE
    return ResultStringCtor((String){}, err);
}

const char* getCompiler()
{
    ERROR_CHECKING();

    const char* compiler = getenv("CC");
    if (!compiler)
    {
        HANDLE_ERRNO_ERROR("Error getenv $(CC): %s");
    }

ERROR_CASE
    return compiler;
}
