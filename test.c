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

#define assert_sizet_eq(real, expected)                                                            \
    do {                                                                                           \
        if((real) != (expected)) {                                                                 \
            printf("expected result %zu but got %zu\n", (expected), (real));                       \
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

#define assert_ptr_neq(real, expected)                                                             \
    do {                                                                                           \
        if((real) == (expected)) {                                                                 \
            printf("expected ptr %p not to be %p\n", (real), (expected));                          \
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
    printf("%s\n", ac_command_help(&command1));
    assert_int_eq(ac_command_validate(&command1).code, AC_ERROR_SUCCESS);

    char const *const                   argv1[]     = {};
    struct ac_command                   args        = {0};
    struct ac_command_spec const *const command_ptr = &command1;
    struct ac_status                    result = ac_command_parse(0, argv1, command_ptr, &args);
    assert_int_eq(result.code, AC_ERROR_SUCCESS);
    assert_ptr_eq(args.command, command_ptr);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv2[] = {"--value"};
    result                    = ac_command_parse(1, argv2, command_ptr, &args);
    assert_int_eq(result.code, AC_ERROR_OPTION_NAME_NOT_IN_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv3[] = {"name2"};
    result                    = ac_command_parse(1, argv3, command_ptr, &args);
    assert_int_eq(result.code, AC_ERROR_ARGUMENT_EXCEEDED_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);
}

static struct ac_command_spec const command2 = {
    .help      = "Testing command 2.",
    .n_options = 2,
    .options   = (struct ac_option_spec[]) {{
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
                                                .help           = "change in the number of bananas",
                                          }}};

static void test_command_2() {
    printf("%s\n", ac_command_help(&command2));
    assert_int_eq(ac_command_validate(&command2).code, AC_ERROR_SUCCESS);

    char const *const                   argv1[]     = {};
    struct ac_command                   args        = {0};
    struct ac_command_spec const *const command_ptr = &command2;
    struct ac_status                    result      = ac_command_parse(0, argv1, &command2, &args);
    assert_int_eq(result.code, AC_ERROR_OPTION_NAME_REQUIRED_IN_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv2[] = {"--apple"};
    result                    = ac_command_parse(1, argv2, &command2, &args);
    assert_int_eq(result.code, AC_ERROR_OPTION_VALUE_EXPECTED);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv3[] = {"--apple", "mmm"};
    result                    = ac_command_parse(2, argv3, &command2, &args);
    assert_int_eq(result.code, AC_ERROR_SUCCESS);
    assert_ptr_eq(args.command, command_ptr);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 1UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_neq(args.options, NULL);
    assert_str_eq(args.options->value, "mmm");

    char const *const argv4[] = {"-a", "mmm"};
    result                    = ac_command_parse(2, argv4, &command2, &args);
    assert_int_eq(result.code, AC_ERROR_SUCCESS);
    assert_ptr_eq(args.command, command_ptr);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 1UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_neq(args.options, NULL);
    assert_str_eq(args.options->value, "mmm");

    char const *const argv5[] = {"-a", "-b"};
    result                    = ac_command_parse(2, argv5, &command2, &args);
    assert_int_eq(result.code, AC_ERROR_OPTION_VALUE_EXPECTED);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv6[] = {"-b", "bbb"};
    result                    = ac_command_parse(2, argv6, &command2, &args);
    assert_int_eq(result.code, AC_ERROR_OPTION_NAME_REQUIRED_IN_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv7[] = {"--dragon", "dddd"};
    result                    = ac_command_parse(2, argv7, &command2, &args);
    assert_int_eq(result.code, AC_ERROR_OPTION_NAME_NOT_IN_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv8[] = {"", "dddd"};
    result                    = ac_command_parse(2, argv7, &command2, &args);
    assert_int_eq(result.code, AC_ERROR_OPTION_NAME_NOT_IN_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);
}

static struct ac_command_spec const command3 = {.help        = "Testing command 3.",
                                                .n_arguments = 2,
                                                .arguments =
                                                    (struct ac_argument_spec[]) {
                                                        {
                                                            .name = "FILE",
                                                            .help = "A file path",
                                                        },
                                                        {
                                                            .name = "OUTPUT",
                                                            .help = "An output path",
                                                        },
                                                    },
                                                .n_options = 3,
                                                .options   = (struct ac_option_spec[]) {
                                                    {
                                                          .long_name      = "apple",
                                                          .has_short_name = true,
                                                          .short_name     = 'a',
                                                          .help           = "number of apples",
                                                    },
                                                    {
                                                          .long_name      = "banana",
                                                          .has_short_name = true,
                                                          .short_name     = 'b',
                                                          .help = "change in the number of bananas",
                                                    },
                                                    {
                                                          .long_name      = "cherry",
                                                          .has_short_name = true,
                                                          .short_name     = 'c',
                                                          .is_flag        = true,
                                                          .help = "are there cherries?",
                                                    },
                                                }};

static void test_command_3() {
    printf("%s\n", ac_command_help(&command3));
    assert_int_eq(ac_command_validate(&command3).code, AC_ERROR_SUCCESS);

    char const *const                   argv1[]     = {};
    struct ac_command                   args        = {0};
    struct ac_command_spec const *const command_ptr = &command3;
    struct ac_status                    result      = ac_command_parse(0, argv1, &command3, &args);
    assert_int_eq(result.code, AC_ERROR_ARGUMENT_EXPECTED_IN_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv2[] = {"/path/to/a", "/path/to/b"};
    result                    = ac_command_parse(2, argv2, &command3, &args);
    assert_int_eq(result.code, AC_ERROR_SUCCESS);
    assert_ptr_eq(args.command, command_ptr);
    assert_sizet_eq(args.n_arguments, 2UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_neq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv3[] = {"/path/to/a", "/path/to/b", "/path/to/c"};
    result                    = ac_command_parse(3, argv3, &command3, &args);
    assert_int_eq(result.code, AC_ERROR_ARGUMENT_EXCEEDED_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv4[] = {"/path/to/a", "/path/to/b", "--banana", "5"};
    result                    = ac_command_parse(4, argv4, &command3, &args);
    assert_int_eq(result.code, AC_ERROR_SUCCESS);
    assert_ptr_eq(args.command, command_ptr);
    assert_sizet_eq(args.n_arguments, 2UL);
    assert_sizet_eq(args.n_options, 1UL);
    assert_ptr_neq(args.arguments, NULL);
    assert_ptr_neq(args.options, NULL);

    char const *const argv5[] = {"/path/to/a", "/path/to/b", "-c"};
    result                    = ac_command_parse(3, argv5, &command3, &args);
    assert_int_eq(result.code, AC_ERROR_SUCCESS);
    assert_ptr_eq(args.command, command_ptr);
    assert_sizet_eq(args.n_arguments, 2UL);
    assert_sizet_eq(args.n_options, 1UL);
    assert_ptr_neq(args.arguments, NULL);
    assert_ptr_neq(args.options, NULL);
}

static struct ac_multi_command_spec const command4 = {
    .help          = "Multiple commands",
    .n_subcommands = 3,
    .subcommands   = (struct ac_multi_command_subcommand[]) {
        {
              .name           = "command1",
              .type           = COMMAND_SINGLE,
              .single.command = (struct ac_command_spec *) &command1,
        },
        {
              .name           = "command2",
              .type           = COMMAND_SINGLE,
              .single.command = (struct ac_command_spec *) &command2,
        },
        {.name = "subcommand3",
           .type = COMMAND_MULTI,
           .multi.subcommands =
               (struct ac_multi_command_spec[]) {
                 {.help          = "do subcommand3",
                    .n_subcommands = 1,
                    .subcommands =
                        (struct ac_multi_command_subcommand[]) {
                          {.name = "command3",
                             .type = COMMAND_SINGLE,
                             .single =
                                 {
                                     .command = (struct ac_command_spec *) &command3,
                               }}

                      }

                 }}

        }}};

static void test_command_4() {
    printf("%s\n", ac_multi_command_help(&command4));
    assert_int_eq(ac_multi_command_validate(&command4).code, AC_ERROR_SUCCESS);

    char const *const argv1[] = {""};
    struct ac_command args    = {0};
    struct ac_status  result  = ac_multi_command_parse(1, argv1, &command4, &args);
    assert_int_eq(result.code, AC_ERROR_COMMAND_NAME_INVALID);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv2[] = {"subcommand3"};
    result                    = ac_multi_command_parse(1, argv2, &command4, &args);
    assert_int_eq(result.code, AC_ERROR_COMMAND_NAME_REQUIRED);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv3[] = {"blah"};
    result                    = ac_multi_command_parse(1, argv3, &command4, &args);
    assert_int_eq(result.code, AC_ERROR_COMMAND_NAME_NOT_IN_SPEC);
    assert_ptr_eq(args.command, NULL);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv4[] = {"command1"};
    result                    = ac_multi_command_parse(1, argv4, &command4, &args);
    assert_int_eq(result.code, AC_ERROR_SUCCESS);
    assert_ptr_eq(args.command, &command1);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 0UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_eq(args.options, NULL);

    char const *const argv5[] = {"command2", "--apple", "5"};
    result                    = ac_multi_command_parse(3, argv5, &command4, &args);
    assert_int_eq(result.code, AC_ERROR_SUCCESS);
    assert_ptr_eq(args.command, &command2);
    assert_sizet_eq(args.n_arguments, 0UL);
    assert_sizet_eq(args.n_options, 1UL);
    assert_ptr_eq(args.arguments, NULL);
    assert_ptr_neq(args.options, NULL);

    char const *const argv6[] = {"subcommand3", "command3", "/path/to/a",
                                 "/path/to/b",  "--banana", "10"};
    result                    = ac_multi_command_parse(6, argv6, &command4, &args);
    assert_int_eq(result.code, AC_ERROR_SUCCESS);
    assert_ptr_eq(args.command, &command3);
    assert_sizet_eq(args.n_arguments, 2UL);
    assert_sizet_eq(args.n_options, 1UL);
    assert_ptr_neq(args.arguments, NULL);
    assert_ptr_neq(args.options, NULL);
}

int main() {
    test_command_1();
    test_command_2();
    test_command_3();
    test_command_4();
}