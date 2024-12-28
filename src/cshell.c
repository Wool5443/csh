#include <limits.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cshell.h"
#include "Logger.h"
#include "Vector.h"
#include "String.h"
#include "IOUtils.h"

typedef const char** ArgsList;
DECLARE_RESULT(ArgsList);

typedef const char* Path;
DECLARE_RESULT(Path);

ResultString cleanInterpreterComment(const Str sourcePath);

const char* getCompiler();
ResultArgsList argsListCtor(const char outputPath[static 1], const char* args[]);
ErrorCode compile(ArgsList compileArgs);
ErrorCode run(ArgsList runArgs);
ErrorCode delete(const char path[static 1]);


ErrorCode CompileAndRunFile(const char path[static 1], const char* args[])
{
    ERROR_CHECKING();

    assert(path);

    String sourcePath = {};
    String cleanSourcePath = {};
    String executablePath = {};
    ArgsList runArgs = {};

    ResultString sourcePathResult = RealPath(path);
    CHECK_ERROR(sourcePathResult.errorCode);
    sourcePath = sourcePathResult.value;

    ResultString cleanSourcePathResult = cleanInterpreterComment(StrCtorFromString(sourcePath));
    CHECK_ERROR(cleanSourcePathResult.errorCode);
    cleanSourcePath = cleanSourcePathResult.value;

    ResultString executablePathResult = StringCopy(cleanSourcePath);
    CHECK_ERROR(executablePathResult.errorCode);
    executablePath = executablePathResult.value;

    CHECK_ERROR(StringAppend(&executablePath, ".exe"));

    const char* compileArgs[] = {
        getCompiler(),
        cleanSourcePath.data,
        "-O3",
        "-o",
        executablePath.data,
        NULL,
    };

    ResultArgsList runArgsResult = argsListCtor(executablePath.data, args);
    CHECK_ERROR(runArgsResult.errorCode);
    runArgs = runArgsResult.value;

    CHECK_ERROR(compile(compileArgs));
    CHECK_ERROR(run(runArgs));
    CHECK_ERROR(delete(executablePath.data));
    CHECK_ERROR(delete(cleanSourcePath.data));

ERROR_CASE
    VecDtor(runArgs);
    StringDtor(&sourcePath);
    StringDtor(&executablePath);
    StringDtor(&cleanSourcePath);

    return err;
}

const char* getCompiler()
{
    ERROR_CHECKING();

    const char* compiler = getenv("CC");
    if (!compiler)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error getenv $(ERROR_LINUX, CC): %s");
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
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error vfork: %s");
    }
    else if (compile == 0)
    {
        execvp(compileArgs[0], (char**)compileArgs);

        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error execvp: %s");
    }

    if (wait(NULL) == -1)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error wait: %s");
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
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error vfork: %s");
    }
    else if (run == 0)
    {
        execvp(runArgs[0], (char**)runArgs);

        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error execvp: %s");
    }

    if (wait(NULL) == -1)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error wait: %s");
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
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error vfork: %s");
    }
    else if (delete == 0)
    {
        execvp(deleteArgs[0], (char**)deleteArgs);

        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error execvp: %s");
    }

    if (wait(NULL) == -1)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error wait: %s");
    }

ERROR_CASE
    return err;
}

ResultString cleanInterpreterComment(const Str sourcePath)
{
    ERROR_CHECKING();

    assert(sourcePath.data);

    String sourceFile = {};
    String newSourceFilePath = {};
    FILE* newSourceFile = NULL;

    ResultString sourceFileResult = StringReadFile(sourcePath.data);
    CHECK_ERROR(sourceFileResult.errorCode);
    sourceFile = sourceFileResult.value;

    Str sourceFolder = GetFolderStr(sourcePath);

    ResultString newSourceFilePathResult = StringCtorFromStr(sourceFolder);
    CHECK_ERROR(newSourceFilePathResult.errorCode);
    newSourceFilePath = newSourceFilePathResult.value;

    CHECK_ERROR(StringAppend(&newSourceFilePath, ".TEMP_RUN_"));

    // after folder comes file name
    CHECK_ERROR(StringAppend(&newSourceFilePath, sourceFolder.data + sourceFolder.size));

    char* commentPtr = strstr(sourceFile.data, "#!");
    if (commentPtr)
    {
        commentPtr[0] = '/';
        commentPtr[1] = '/';
    }

    newSourceFile = fopen(newSourceFilePath.data, "w");
    if (!newSourceFile)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error fopen: %s");
    }
    fwrite(sourceFile.data, 1, sourceFile.size, newSourceFile);

ERROR_CASE
    StringDtor(&sourceFile);
    if (newSourceFile) fclose(newSourceFile);

    if (err == EVERYTHING_FINE)
    {
        return ResultStringCtor(newSourceFilePath, EVERYTHING_FINE);
    }

    StringDtor(&newSourceFilePath);

    return ResultStringCtor((String){}, err);
}
