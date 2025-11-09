#include "args-c.h"

#include <assert.h>
#include <stdio.h>

#define assert_int_eq(real, expected)                                                              \
    do {                                                                                           \
        if((real) != (expected)) {                                                                 \
            printf("expected result %d but got %d\n", (expected), (real));                         \
            assert(false);                                                                         \
        }                                                                                          \
    } while(false)

#define assert_sizet_eq(real, expected)                                                              \
    do {                                                                                           \
        if((real) != (expected)) {                                                                 \
            printf("expected result %zu but got %zu\n", (expected), (real));                         \
            assert(false);                                                                         \
        }                                                                                          \
    } while(false)

#define assert_ptr_eq(real, expected)                                                              \
    do {                                                                                           \
        if((real) != (expected)) {                                                                 \
            printf("expected ptr %p but got %p\n", (expected), (real));                            \
            assert(false);                                                                         \
        }                                                                                          \
    } while(false)

#define assert_ptr_neq(real, expected)                                                              \
    do {                                                                                           \
        if((real) == (expected)) {                                                                 \
            printf("expected ptr %p not to be %p\n", (real), (expected));                            \
            assert(false);                                                                         \
        }                                                                                          \
    } while(false)

#define assert_str_eq(real, expected)                                                              \
    do {                                                                                           \
        if(0 != strcmp(real, expected)) {                                                          \
            printf("expected ptr %s but got %s\n", (expected), (real));                            \
            assert(false);                                                                         \
        }                                                                                          \
    } while(false)

static struct ac_command_spec const command1 = {
    .help        = "Testing command 1.",
    .n_arguments = 0,
};

static void test_command_1() {
    char const *const                   argv1[]     = {};
    struct ac_command                   args        = {0};
    struct ac_command_spec const *const command_ptr = &command1;
    enum ac_error                       result = ac_parse_command(0, argv1, command_ptr, &args);
    assert_int_eq(result, AC_ERROR_SUCCESS);
    assert_ptr_eq(args.command, command_ptr);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv2[] = {"--value"};
    result                    = ac_parse_command(1, argv2, command_ptr, &args);
    assert_int_eq(result, AC_ERROR_OPTION_NAME_NOT_IN_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv3[] = {"name2"};
    result                    = ac_parse_command(1, argv3, command_ptr, &args);
    assert_int_eq(result, AC_ERROR_ARGUMENT_EXCEEDED_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);
}

static struct ac_command_spec const command2 = {.help      = "Testing command 2.",
                                                .n_options = 2,
                                                .options   = (struct ac_option_spec[]) {
                                                    {
                                                          .long_name      = "apple",
                                                          .has_short_name = true,
                                                          .short_name     = 'a',
                                                          .help           = "number of apples",
                                                          .required       = true,
                                                    },
                                                    {
                                                          .long_name      = "banana",
                                                          .has_short_name = true,
                                                          .short_name     = 'b',
                                                          .help = "change in the number of bananas",
                                                    }
                                                }};

static void test_command_2() {
    char const *const argv1[] = {};
    struct ac_command args    = {0};
    struct ac_command_spec const *const command_ptr = &command2;
    enum ac_error     result  = ac_parse_command(0, argv1, &command2, &args);
    assert_int_eq(result, AC_ERROR_OPTION_NAME_REQUIRED_IN_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv2[] = {"--apple"};
    result  = ac_parse_command(1, argv2, &command2, &args);
    assert_int_eq(result, AC_ERROR_OPTION_VALUE_EXPECTED);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv3[] = {"--apple", "mmm"};
    result  = ac_parse_command(2, argv3, &command2, &args);
    assert_int_eq(result, AC_ERROR_SUCCESS);
    assert_ptr_eq(args.command, command_ptr);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 1UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_neq(args.options, NULL);
    assert_str_eq(args.options->value, "mmm");

    char const *const argv4[] = {"-a", "mmm"};
    result  = ac_parse_command(2, argv4, &command2, &args);
    assert_int_eq(result, AC_ERROR_SUCCESS);
    assert_ptr_eq(args.command, command_ptr);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 1UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_neq(args.options, NULL);
    assert_str_eq(args.options->value, "mmm");

    char const *const argv5[] = {"-a", "-b"};
    result  = ac_parse_command(2, argv5, &command2, &args);
    assert_int_eq(result, AC_ERROR_OPTION_VALUE_EXPECTED);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv6[] = {"-b", "bbb"};
    result  = ac_parse_command(2, argv6, &command2, &args);
    assert_int_eq(result, AC_ERROR_OPTION_NAME_REQUIRED_IN_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv7[] = {"--dragon", "dddd"};
    result  = ac_parse_command(2, argv7, &command2, &args);
    assert_int_eq(result, AC_ERROR_OPTION_NAME_NOT_IN_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv8[] = {"", "dddd"};
    result  = ac_parse_command(2, argv7, &command2, &args);
    assert_int_eq(result, AC_ERROR_OPTION_NAME_NOT_IN_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);
}

int main() {
    test_command_1();
    test_command_2();
}