#include "GatherArgs.h"

static const char* getCompiler();
static ErrorCode gatherCompilerArgsImpl(const Str argType, CompilerArgsList* args, String sourceFile);

ResultCompilerArgsList
GatherCompilerArgs(const Str sourcePath, const Str executablePath)
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

ResultArgsList
GatherCommandLineArgs(const char outputPath[static 1],
                      const int argc, const char* args[])
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

static const char* getCompiler()
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

static ErrorCode gatherCompilerArgsImpl(const Str argType, CompilerArgsList* args, String sourceFile)
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

