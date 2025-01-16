#ifndef CHELL_COMPILER_ARGS_LIST_H
#define CHELL_COMPILER_ARGS_LIST_H

#include "String.h"

typedef String* CompilerArgsList;
DECLARE_RESULT_HEADER(CompilerArgsList);

void CompilerArgsListDtor(CompilerArgsList list);

#endif // CHELL_COMPILER_ARGS_LIST_H
