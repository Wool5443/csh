#include "CompilerArgsList.h"
#include "Vector.h"

DECLARE_RESULT_SOURCE(CompilerArgsList);

void CompilerArgsListDtor(CompilerArgsList list)
{
    size_t size = VecSize(list);

    for (size_t i = 0; i < size; i++)
    {
        StringDtor(&list[i]);
    }

    VecDtor(list);
}
