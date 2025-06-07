#include <limits.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cshell.h"
#include "Logger.h"
#include "Vector.h"
#include "String.h"
#include "IO.h"
#include "Gather_args.h"

typedef const char* Path;
DECLARE_RESULT_HEADER(Path);
DECLARE_RESULT_SOURCE(Path);

static Result_String clean_interpreter_comment(const Str source_path);

static Error_code compile(Compiler_args_list compiler_args);
static Error_code run(Args_list run_args);
static Error_code delete(const char* path);

Error_code compile_and_run_file(const char* path, const int argc, const char* args[])
{
    ERROR_CHECKING();

    assert(path);

    String source_path = {};
    String clean_source_path = {};
    String executable_path = {};
    Args_list run_args = {};
    Compiler_args_list compiler_args = {};

    Result_String source_path_result = real_path(path);
    CHECK_ERROR(source_path_result.error_code);
    source_path = source_path_result.value;

    Result_String clean_source_path_result = clean_interpreter_comment(str_ctor_string(source_path));
    CHECK_ERROR(clean_source_path_result.error_code);
    clean_source_path = clean_source_path_result.value;

    Result_String executable_path_result = string_copy(clean_source_path);
    CHECK_ERROR(executable_path_result.error_code);
    executable_path = executable_path_result.value;

    CHECK_ERROR(string_append(&executable_path, ".exe"));

    Result_Args_list run_args_result = gather_command_line_args(executable_path.data, argc, args);
    CHECK_ERROR(run_args_result.error_code);
    run_args = run_args_result.value;

    Result_Compiler_args_list compiler_args_result = gather_compiler_args(
        str_ctor_string(clean_source_path),
        str_ctor_string(executable_path)
    );
    CHECK_ERROR(compiler_args_result.error_code);
    compiler_args = compiler_args_result.value;

    CHECK_ERROR(compile(compiler_args));
    CHECK_ERROR(run(run_args));
    CHECK_ERROR(delete(executable_path.data));
    CHECK_ERROR(delete(clean_source_path.data));

ERROR_CASE
    vec_dtor(run_args);
    compiler_args_list_dtor(compiler_args);
    string_dtor(&source_path);
    string_dtor(&executable_path);
    string_dtor(&clean_source_path);

    return err;
}

Error_code compile(Compiler_args_list compiler_args)
{
    ERROR_CHECKING();

    assert(compiler_args);

    size_t args_num = vec_size(compiler_args);
    char** args = {};
    vec_reserve(args, args_num);

    for (size_t i = 0; i < args_num; i++)
    {
        CHECK_ERROR(vec_add(args, compiler_args[i].data));
    }

    for (size_t i = 0; i < args_num; i++)
    {
        printf("%s ", args[i]);
    }
    printf("\n\n");

    pid_t compile_pid = vfork();

    if (compile_pid == -1)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error vfork: %s");
    }
    else if (compile_pid == 0)
    {
        execvp(args[0], args);

        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error execvp: %s");
    }

    int ret_code = 0;

    if (wait(&ret_code) == -1)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error wait: %s");
    }

    if (ret_code != 0)
    {
        err = ERROR_BAD_FILE;

        printf("\n\nFailed to compile!!!\n");

        ERROR_LEAVE();
    }

ERROR_CASE
    vec_dtor(args);

    return err;
}

static Error_code run(Args_list run_args)
{
    ERROR_CHECKING();

    assert(run_args);

    pid_t run_pid = vfork();

    if (run_pid == -1)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error vfork: %s");
    }
    else if (run_pid == 0)
    {
        execvp(run_args[0], (char**)run_args);

        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error execvp: %s");
    }

    if (wait(NULL) == -1)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error wait: %s");
    }

ERROR_CASE
    return err;
}

static Error_code delete(const char* path)
{
    ERROR_CHECKING();

    assert(path);

    const char* delete_args[] = {
        "rm",
        path,
        NULL,
    };
    pid_t delete_pid = vfork();

    if (delete_pid == -1)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error vfork: %s");
    }
    else if (delete_pid == 0)
    {
        execvp(delete_args[0], (char**)delete_args);

        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error execvp: %s");
    }

    if (wait(NULL) == -1)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error wait: %s");
    }

ERROR_CASE
    return err;
}

static Result_String clean_interpreter_comment(const Str source_path)
{
    ERROR_CHECKING();

    assert(source_path.data);

    String source_file = {};
    String new_source_file_path = {};
    FILE* new_source_file = NULL;

    Result_String source_file_result = read_file(source_path.data);
    CHECK_ERROR(source_file_result.error_code);
    source_file = source_file_result.value;

    Result_Str source_folder_result = get_folder_str(source_path);
    CHECK_ERROR(source_folder_result .error_code);
    Str source_folder = source_folder_result.value;

    Result_String new_source_file_path_result = string_ctor_str(source_folder);
    CHECK_ERROR(new_source_file_path_result.error_code);
    new_source_file_path = new_source_file_path_result.value;

    CHECK_ERROR(string_append(&new_source_file_path, ".TEMP_RUN_"));

    // after folder comes file name
    CHECK_ERROR(string_append(&new_source_file_path, source_folder.data + source_folder.size));

    char* comment_ptr = strstr(source_file.data, "#!");
    if (comment_ptr)
    {
        comment_ptr[0] = '/';
        comment_ptr[1] = '/';
    }

    new_source_file = fopen(new_source_file_path.data, "w");
    if (!new_source_file)
    {
        HANDLE_ERRNO_ERROR(ERROR_LINUX, "Error fopen: %s");
    }
    fwrite(source_file.data, 1, source_file.size, new_source_file);

ERROR_CASE
    string_dtor(&source_file);
    if (new_source_file) fclose(new_source_file);

    if (err == EVERYTHING_FINE)
    {
        return Result_String_ctor(new_source_file_path, EVERYTHING_FINE);
    }

    string_dtor(&new_source_file_path);

    return Result_String_ctor((String){}, err);
}
