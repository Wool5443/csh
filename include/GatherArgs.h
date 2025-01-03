#ifndef CSHELL_GATHER_COMPILE_ARGS_H
#define CSHELL_GATHER_COMPILE_ARGS_H

#include "CompilerArgsList.h"

typedef const char** ArgsList;
DECLARE_RESULT(ArgsList);

static const char COMPILER_ARG_QUOTE = '"';

ResultCompilerArgsList
GatherCompilerArgs(const Str sourcePath, const Str executablePath);


ResultArgsList
GatherCommandLineArgs(const char outputPath[static 1],
                      const int argc, const char* args[]);

#endif // CSHELL_GATHER_COMPILE_ARGS_H
