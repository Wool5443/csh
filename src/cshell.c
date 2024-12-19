#include <limits.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cshell.h"
#include "Logger.h"
#include "Vector.h"
#include "String.h"

typedef const char** ArgsList;
DECLARE_RESULT(ArgsList);

typedef const char* Path;
DECLARE_RESULT(Path);

static const Str TEMP_FOLDER = { "/tmp/", strlen(TEMP_FOLDER.data) };

ResultString sanitizeFilePath(const char path[static 1]);
const char* getCompiler();
ResultArgsList argsListCtor(const char outputPath[static 1], const char* args[]);
ErrorCode compile(ArgsList compileArgs);
ErrorCode run(ArgsList runArgs);
ErrorCode delete(const char path[static 1]);
ResultString cleanInterpreterComment(const char path[static 1]);

ErrorCode CompileAndRunFile(const char path[static 1], const char* args[])
{
    ERROR_CHECKING();

    assert(path);

    String sourcePath = {};
    String cleanSourcePath = {};
    String outputPath = {};
    ArgsList runArgs = {};

    ResultString sourcePathResult = sanitizeFilePath(path);
    CHECK_ERROR(sourcePathResult.errorCode);
    sourcePath = sourcePathResult.value;

    ResultString cleanSourcePathResult = cleanInterpreterComment(path);
    CHECK_ERROR(cleanSourcePathResult.errorCode);
    cleanSourcePath = cleanSourcePathResult.value;

    ResultString outputPathResult = StringCtorFromStr(TEMP_FOLDER);
    CHECK_ERROR(outputPathResult.errorCode);
    outputPath = outputPathResult.value;

    CHECK_ERROR(StringAppendString(&outputPath, sourcePath));
    CHECK_ERROR(StringAppend(&outputPath, ".exe"));

    char* slash = strchr(outputPath.data + TEMP_FOLDER.size, '/');
    while (slash)
    {
        *slash = '*';
        slash = strchr(slash + 1, '/');
    }

    const char* compileArgs[] = {
        getCompiler(),
        cleanSourcePath.data,
        "-O3",
        "-o",
        outputPath.data,
        NULL,
    };

    ResultArgsList runArgsResult = argsListCtor(outputPath.data, args);
    CHECK_ERROR(runArgsResult.errorCode);
    runArgs = runArgsResult.value;

    CHECK_ERROR(compile(compileArgs));
    CHECK_ERROR(run(runArgs));
    CHECK_ERROR(delete(outputPath.data));
    CHECK_ERROR(delete(cleanSourcePath.data));

ERROR_CASE
    VecDtor(runArgs);
    StringDtor(&sourcePath);
    StringDtor(&outputPath);
    StringDtor(&cleanSourcePath);

    return err;
}

ResultString sanitizeFilePath(const char path[static 1])
{
    ERROR_CHECKING();

    assert(path);

    char goodPath[PATH_MAX + 1] = "";

    if (!realpath(path, goodPath))
    {
        HANDLE_ERRNO_ERROR("Error realpath for %s: %s", path);
    }

    return StringCtorFromStr(StrCtorSize(goodPath, strlen(goodPath)));

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

ResultArgsList argsListCtor(const char outputPath[static 1], const char* args[])
{
    ERROR_CHECKING();

    assert(args);

    ArgsList list = {};

    const char** arg = args;
    CHECK_ERROR(VecAdd(list, outputPath));

    while (*arg)
    {
        CHECK_ERROR(VecAdd(list, *arg), "Error adding arg: %s", *arg);
        arg++;
    }
    CHECK_ERROR(VecAdd(list, NULL), "Error adding NULL");

    return ResultArgsListCtor(list, EVERYTHING_FINE);

ERROR_CASE
    VecDtor(list);

    return ResultArgsListCtor((ArgsList){}, err);
}

ErrorCode compile(ArgsList compileArgs)
{
    ERROR_CHECKING();

    assert(compileArgs);

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

ERROR_CASE
    return err;
}

ErrorCode run(ArgsList runArgs)
{
    ERROR_CHECKING();

    assert(runArgs);

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

ERROR_CASE
    return err;
}

ErrorCode delete(const char path[static 1])
{
    ERROR_CHECKING();

    assert(path);

    const char* deleteArgs[] = {
        "rm",
        path,
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

    if (wait(NULL) == -1)
    {
        HANDLE_ERRNO_ERROR("Error wait: %s");
    }

ERROR_CASE
    return err;
}

ResultString cleanInterpreterComment(const char path[static 1])
{
    ERROR_CHECKING();

    assert(path);

    String file = {};
    String newFilePath = {};
    FILE* outFile = NULL;

    ResultString fileResult = StringReadFile(path);
    CHECK_ERROR(fileResult.errorCode);
    file = fileResult.value;

    char* commentPtr = strstr(file.data, "#!");
    if (commentPtr)
    {
        commentPtr[0] = '/';
        commentPtr[1] = '/';
    }

    CHECK_ERROR(StringAppend(&newFilePath, TEMP_FOLDER.data));
    CHECK_ERROR(StringAppend(&newFilePath, path));

    char* slash = strchr(newFilePath.data + TEMP_FOLDER.size, '/');
    while (slash)
    {
        *slash = '*';
        slash = strchr(slash + 1, '/');
    }

    outFile = fopen(newFilePath.data, "w");
    if (!outFile)
    {
        HANDLE_ERRNO_ERROR("Error fopen: %s");
    }
    fwrite(file.data, 1, file.size, outFile);

ERROR_CASE
    StringDtor(&file);
    if (outFile) fclose(outFile);

    if (err == EVERYTHING_FINE)
    {
        return ResultStringCtor(newFilePath, EVERYTHING_FINE);
    }

    StringDtor(&newFilePath);

    return ResultStringCtor((String){}, err);
}
