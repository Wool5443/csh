#ifndef CHELL_COMPILER_ARGS_LIST_H
#define CHELL_COMPILER_ARGS_LIST_H

#include "String.h"
#include "Vector.h"

typedef String* CompilerArgsList;
DECLARE_RESULT(CompilerArgsList);

INLINE void CompilerArgsListDtor(CompilerArgsList list)
{
    size_t size = VecSize(list);

    for (size_t i = 0; i < size; i++)
    {
        StringDtor(&list[i]);
    }

    VecDtor(list);
}

#endif // CHELL_COMPILER_ARGS_LIST_H
