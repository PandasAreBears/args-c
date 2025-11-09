#include "args-c.h"

#include <assert.h>
#include <stdio.h>

#define assert_error_type(real, expected)                                                          \
    do {                                                                                           \
        if((real) != (expected)) {                                                                 \
            printf("expected result %d but got %d\n", (expected), (real));                         \
            assert(false);                                                                         \
        }                                                                                          \
    } while(false)

static struct ac_command_spec const command1 = {
    .name = "command1",
    .help = "Testing command 1.",
    .type = COMMAND_TERMINAL,
    .terminal =
        {
            .n_arguments = 0,
        },
};

static void test_command_1() {
    char const *const          argv1[] = {"command1"};
    struct ac_arguments const *args    = NULL;
    enum ac_error              result  = ac_parse_argv(1, argv1, &command1, &args);
    assert_error_type(result, AC_ERROR_SUCCESS);

    char const *const argv2[] = {"command1", "--value"};
    result                    = ac_parse_argv(2, argv2, &command1, &args);
    assert_error_type(result, AC_ERROR_COMMAND_NAME_INVALID);

    char const *const argv3[] = {"command1", "name2"};
    result                    = ac_parse_argv(2, argv3, &command1, &args);
    assert_error_type(result, AC_ERROR_TOO_MANY_COMMAND_NAMES);

    char const *const argv4[] = {};
    result                    = ac_parse_argv(0, argv4, &command1, &args);
    assert_error_type(result, AC_ERROR_INVALID_PARAMETER);
}

static struct ac_command_spec const command2 = {
    .name = "command2",
    .help = "Testing command 2.",
    .type = COMMAND_TERMINAL,
    .terminal =
        {
            .n_arguments = 5,
            .arguments =
                (struct ac_argument_spec[]) {
                    {
                        .long_name      = "apple",
                        .has_short_name = true,
                        .short_name     = 'a',
                        .help           = "number of apples",
                        .required       = true,
                        .type           = ARGUMENT_TYPE_UNSIGNED_INTEGER,
                    },
                    {
                        .long_name      = "banana",
                        .has_short_name = true,
                        .short_name     = 'b',
                        .help           = "change in the number of bananas",
                        .type           = ARGUMENT_TYPE_SIGNED_INTEGER,
                    },
                    {
                        .long_name      = "cherry",
                        .help           = "the type of cherry",
                        .type           = ARGUMENT_TYPE_STRING,
                    },
                },
        },
};

static void test_command_2() {

}

int main() {
    test_command_1();
    test_command_2();
}