#ifndef CSHELL_GATHER_COMPILE_ARGS_H
#define CSHELL_GATHER_COMPILE_ARGS_H

#include "Compiler_args_list.h"

typedef const char** Args_list;
DECLARE_RESULT_HEADER(Args_list);

static const char COMPILER_ARG_QUOTE = '"';

Result_Compiler_args_list
gather_compiler_args(const Str source_path, const Str executable_path);


Result_Args_list
gather_command_line_args(const char* output_path,
                         const int argc, const char* args[]);

#endif // CSHELL_GATHER_COMPILE_ARGS_H
