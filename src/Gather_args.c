#include "Gather_args.h"
#include "Vector.h"

DECLARE_RESULT_SOURCE(Args_list);

static const char* get_compiler();
static Error_code gather_compiler_args_impl(const Str arg_type, Compiler_args_list* args, String source_file);

Result_Compiler_args_list
gather_compiler_args(const Str source_path, const Str executable_path)
{
    static const char* ARG_TYPES[] = {
        "INCLUDE",
        "LINK",
        "FILES",
        "FLAGS",
    };

    ERROR_CHECKING();

    String source_file = {};
    Compiler_args_list args = {};

    const char* basic_compiler_args[] = {
        get_compiler(),
        source_path.data,
        "-O3",
        "-march=native",
        "-DNDEBUG",
        "-o",
        executable_path.data,
    };

    CHECK_ERROR(vec_reserve(args, ARRAY_SIZE(basic_compiler_args)));

    for (size_t i = 0; i < ARRAY_SIZE(basic_compiler_args); i++)
    {
        Result_String arg_result = string_ctor(basic_compiler_args[i]);
        CHECK_ERROR(arg_result.error_code);
        CHECK_ERROR(vec_add(args, arg_result.value));
    }

    Result_String source_file_result = read_file(source_path.data);
    CHECK_ERROR(source_file_result.error_code);
    source_file = source_file_result.value;

    for (size_t i = 0; i < ARRAY_SIZE(ARG_TYPES); i++)
    {
        CHECK_ERROR(gather_compiler_args_impl(str_ctor(ARG_TYPES[i]), &args, source_file));
    }

    CHECK_ERROR(vec_add(args, (String){}));

    string_dtor(&source_file);

    return Result_Compiler_args_list_ctor(args, EVERYTHING_FINE);

ERROR_CASE
    string_dtor(&source_file);
    compiler_args_list_dtor(args);

    return Result_Compiler_args_list_ctor((Compiler_args_list){}, err);
}

Result_Args_list
gather_command_line_args(const char* output_path,
                         const int argc, const char* args[])
{
    ERROR_CHECKING();

    assert(args);

    Args_list list = {};

    CHECK_ERROR_LOG(vec_reserve(list, argc + 2), "Error expanding args list");

    const char** arg = args;
    CHECK_ERROR_LOG(vec_add(list, output_path));

    while (*arg)
    {
        CHECK_ERROR_LOG(vec_add(list, *arg), "Error adding arg: %s", *arg);
        arg++;
    }
    CHECK_ERROR_LOG(vec_add(list, NULL), "Error adding NULL");

    return Result_Args_list_ctor(list, EVERYTHING_FINE);

ERROR_CASE
    vec_dtor(list);

    return Result_Args_list_ctor((Args_list){}, err);
}

static const char* get_compiler()
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

static Error_code gather_compiler_args_impl(const Str arg_type, Compiler_args_list* args, String source_file)
{
    ERROR_CHECKING();

    char* flags_name_ptr = strstr(source_file.data, arg_type.data);

    while (flags_name_ptr)
    {
        char* new_line = strchr(flags_name_ptr, '\n');
        if (!new_line)
        {
            HANDLE_ERRNO_ERROR(ERROR_BAD_FILE, "No end of line in file???: %s");
        }
        *new_line = '\0';

        char* current = strchr(flags_name_ptr, COMPILER_ARG_QUOTE);

        while (current)
        {
            current++;

            char* end_arg = strchr(current, COMPILER_ARG_QUOTE);

            Str arg_str = str_ctor_size(current, end_arg - current);

            Result_String arg_result = string_ctor_str(arg_str);
            CHECK_ERROR(arg_result.error_code);

            if ((err = vec_add(*args, arg_result.value)))
            {
                log_error("Failed to add arg: %s", arg_result.value.data);
                string_dtor(&arg_result.value);
                ERROR_LEAVE();
            }

            current = strchr(end_arg + 1, COMPILER_ARG_QUOTE);
        }

        *new_line = '\n';
        flags_name_ptr = strstr(flags_name_ptr + 1, arg_type.data);
    }

ERROR_CASE

    return err;
}
