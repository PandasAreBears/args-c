#include <assert.h>
#include "args-c.h"

static struct ac_command_spec const example_command = {
    .help      = "A command for specifying fruit quantities.",
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

int main(int const argc, char const *const argv[]) {
    if(argc <= 1) {
        // The @c ac_command_help function is used for generating a help string. This returns an
        // owned string, so the caller is responsible outputting and freeing the buffer.
        printf("%s\n", ac_command_help(&example_command));
        return -1;
    }

    // Non performance restricted environments should validate that command specification is valid
    // using `ac_command_validate`.
    assert(ac_validate_command(&example_command).code == AC_ERROR_SUCCESS);

    // User input is parsed by calling `ac_parse_command`. The `example_command` specification
    // declares what valid user input is. The result of parsing will be populated in the provided
    // `args` structure.
    struct ac_command      args   = {0};
    struct ac_status const result = ac_parse_command(argc - 1, &argv[1], &example_command, &args);
    if(!ac_status_is_success(result)) {
        // Woops, something went wrong. Check out `result.code` for more detail.
        printf("%s\n", ac_command_help(&example_command));
        return -1;
    }

    // Values are extracted from the `args` result using `ac_extract_option`. Arguments use
    // `ac_extract_argument` instead.
    struct ac_option *value = ac_extract_option(&args, "banana");
    // `value` is NULL if the --banana option was unused.

    // ... do something with the parsed command.

    return 0;
}
