#ifndef CHELL_COMPILER_ARGS_LIST_H
#define CHELL_COMPILER_ARGS_LIST_H

#include "String.h"

typedef String* Compiler_args_list;
DECLARE_RESULT_HEADER(Compiler_args_list);

void compiler_args_list_dtor(Compiler_args_list list);

#endif // CHELL_COMPILER_ARGS_LIST_H
