#include "Compiler_args_list.h"
#include "Vector.h"

DECLARE_RESULT_SOURCE(Compiler_args_list);

void compiler_args_list_dtor(Compiler_args_list list)
{
    size_t size = vec_size(list);

    for (size_t i = 0; i < size; i++)
    {
        string_dtor(&list[i]);
    }

    vec_dtor(list);
}
