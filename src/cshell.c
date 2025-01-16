#include <limits.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cshell.h"
#include "Logger.h"
#include "Vector.h"
#include "String.h"
#include "IO.h"
#include "GatherArgs.h"

typedef const char* Path;
DECLARE_RESULT_HEADER(Path);
DECLARE_RESULT_SOURCE(Path);

ResultString cleanInterpreterComment(const Str sourcePath);

ErrorCode compile(CompilerArgsList compilerArgs);
ErrorCode run(ArgsList runArgs);
ErrorCode delete(const char path[static 1]);

ErrorCode CompileAndRunFile(const char path[static 1], const int argc, const char* args[])
{
    ERROR_CHECKING();

    assert(path);

    String sourcePath = {};
    String cleanSourcePath = {};
    String executablePath = {};
    ArgsList runArgs = {};
    CompilerArgsList compilerArgs = {};

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

    ResultArgsList runArgsResult = GatherCommandLineArgs(executablePath.data, argc, args);
    CHECK_ERROR(runArgsResult.errorCode);
    runArgs = runArgsResult.value;

    ResultCompilerArgsList compilerArgsResult = GatherCompilerArgs(
        StrCtorFromString(cleanSourcePath),
        StrCtorFromString(executablePath)
    );
    CHECK_ERROR(compilerArgsResult.errorCode);
    compilerArgs = compilerArgsResult.value;

    CHECK_ERROR(compile(compilerArgs));
    CHECK_ERROR(run(runArgs));
    CHECK_ERROR(delete(executablePath.data));
    CHECK_ERROR(delete(cleanSourcePath.data));

ERROR_CASE
    VecDtor(runArgs);
    CompilerArgsListDtor(compilerArgs);
    StringDtor(&sourcePath);
    StringDtor(&executablePath);
    StringDtor(&cleanSourcePath);

    return err;
}

ErrorCode compile(CompilerArgsList compilerArgs)
{
    ERROR_CHECKING();

    assert(compilerArgs);

    size_t argsNum = VecSize(compilerArgs);
    char** args = {};
    VecExpand(args, argsNum);

    for (size_t i = 0; i < argsNum; i++)
    {
        CHECK_ERROR(VecAdd(args, compilerArgs[i].data));
    }

    for (size_t i = 0; i < argsNum; i++)
    {
        printf("%s ", args[i]);
    }
    printf("\n\n");

    pid_t compile = vfork();

    if (compile == -1)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error vfork: %s");
    }
    else if (compile == 0)
    {
        execvp(args[0], args);

        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error execvp: %s");
    }

    int retCode = 0;

    if (wait(&retCode) == -1)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error wait: %s");
    }

    if (retCode != 0)
    {
        err = ERROR_BAD_FILE;

        printf("\n\nFailed to compile!!!\n");

        ERROR_LEAVE();
    }


ERROR_CASE
    VecDtor(args);

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
