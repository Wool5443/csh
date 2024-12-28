#include <limits.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ScratchBuf.h"
#include "cshell.h"
#include "Logger.h"
#include "Vector.h"
#include "String.h"
#include "IOUtils.h"

#define ARRAY_SIZE(array) sizeof(array) / sizeof(*(array))

static const char COMPILER_ARG_QUOTE = '"';

typedef const char** ArgsList;
DECLARE_RESULT(ArgsList);

typedef String* CompilerArgsList;
DECLARE_RESULT(CompilerArgsList);

typedef const char* Path;
DECLARE_RESULT(Path);

ResultString cleanInterpreterComment(const Str sourcePath);

const char* getCompiler();
ResultArgsList gatherCommandLineArgs(const char outputPath[static 1], const int argc, const char* args[]);
ErrorCode compile(CompilerArgsList compilerArgs);
ErrorCode run(ArgsList runArgs);
ErrorCode delete(const char path[static 1]);

ResultCompilerArgsList gatherCompilerArgs(const Str sourcePath, const Str executablePath);
ErrorCode gatherCompilerArgsImpl(const Str flagsName, CompilerArgsList* args, String sourceFile);

void CompilerArgsListDtor(CompilerArgsList args);

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

    ResultArgsList runArgsResult = gatherCommandLineArgs(executablePath.data, argc, args);
    CHECK_ERROR(runArgsResult.errorCode);
    runArgs = runArgsResult.value;

    ResultCompilerArgsList compilerArgsResult = gatherCompilerArgs(
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

ResultArgsList gatherCommandLineArgs(const char outputPath[static 1], const int argc, const char* args[])
{
    ERROR_CHECKING();

    assert(args);

    ArgsList list = {};

    CHECK_ERROR(VecExpand(list, argc + 2), "Error expanding args list");

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

ResultCompilerArgsList gatherCompilerArgs(const Str sourcePath, const Str executablePath)
{
    static const char* ARG_TYPES[] = {
        "INCLUDE",
        "LINK",
        "FILES",
    };

    ERROR_CHECKING();

    String sourceFile = {};
    CompilerArgsList args = {};

    const char* basicCompilerArgs[] = {
        getCompiler(),
        sourcePath.data,
        "-O3",
        "-DNDEBUG",
        "-o",
        executablePath.data,
    };

    VecCapacity(args);

    CHECK_ERROR(VecExpand(args, ARRAY_SIZE(basicCompilerArgs)));

    for (size_t i = 0; i < ARRAY_SIZE(basicCompilerArgs); i++)
    {
        ResultString argResult = StringCtor(basicCompilerArgs[i]);
        CHECK_ERROR(argResult.errorCode);
        CHECK_ERROR(VecAdd(args, argResult.value));
    }

    ResultString sourceFileResult = StringReadFile(sourcePath.data);
    CHECK_ERROR(sourceFileResult.errorCode);
    sourceFile = sourceFileResult.value;

    for (size_t i = 0; i < ARRAY_SIZE(ARG_TYPES); i++)
    {
        CHECK_ERROR(gatherCompilerArgsImpl(StrCtor(ARG_TYPES[i]), &args, sourceFile));
    }

    CHECK_ERROR(VecAdd(args, (String){}));

    StringDtor(&sourceFile);

    return ResultCompilerArgsListCtor(args, EVERYTHING_FINE);

ERROR_CASE
    StringDtor(&sourceFile);
    CompilerArgsListDtor(args);

    return ResultCompilerArgsListCtor((CompilerArgsList){}, err);
}

ErrorCode gatherCompilerArgsImpl(const Str argType, CompilerArgsList* args, String sourceFile)
{
    ERROR_CHECKING();

    char* flagsNamePtr = strstr(sourceFile.data, argType.data);

    while (flagsNamePtr)
    {
        char* slashN = strchr(flagsNamePtr, '\n');
        if (!slashN)
        {
            HANDLE_ERRNO_ERROR(ERROR_BAD_FILE, "No end of line in file???: %s");
        }
        *slashN = '\0';

        char* current = strchr(flagsNamePtr, COMPILER_ARG_QUOTE);

        while (current)
        {
            current++;

            // current -> arg

            char* endArg = strchr(current, COMPILER_ARG_QUOTE);

            Str argStr = StrCtorSize(current, endArg - current);

            ResultString argResult = StringCtorFromStr(argStr);
            CHECK_ERROR(argResult.errorCode);

            if ((err = VecAdd(*args, argResult.value)))
            {
                LogError("Failed to add arg: %s", argResult.value.data);
                StringDtor(&argResult.value);
                ERROR_LEAVE();
            }

            current = strchr(endArg + 1, COMPILER_ARG_QUOTE);
        }

        *slashN = '\n';
        flagsNamePtr = strstr(flagsNamePtr + 1, argType.data);
    }

ERROR_CASE

    return err;
}

void CompilerArgsListDtor(CompilerArgsList list)
{
    size_t size = VecSize(list);

    for (size_t i = 0; i < size; i++)
    {
        StringDtor(&list[i]);
    }

    VecDtor(list);
}
