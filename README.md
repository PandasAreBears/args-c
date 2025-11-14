
# Args-c

A header-only command line argument parser for the C programming language with a bunch of features that are missing from `getopt(3)`.

1. Subcommand support.
2. Help message generation.
3. Command validation (optional).

## Terminology

- User input: An array of string provided to args-c to be parsed. Typically in the form of `&argv[1]`.
- Command Spec: The specification parsed to args-c which declaratively describes how to parse user input.
- Argument: A required, implicit value on the command line. Like unix's `cat <file>`.
- Option: A named value on the command line. Like unix's `sed -i <file>`.


## Simple usage

```c
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
        printf("%s", ac_command_help(&example_command));
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
        // Woops, something went wrong. Call `ac_error_string` for a helpful error output.
        printf("%s", ac_error_string(result));
        return -1;
    }

    // Values are extracted from the `args` result using `ac_extract_option`. Arguments use
    // `ac_extract_argument` instead.
    struct ac_option *value = ac_extract_option(&args, "banana");
    // `value` is NULL if the --banana option was unused.

    // ... do something with the parsed command.

    return 0;
}
```

## Multi-command usage

```c

#include "args-c.h"

// Multi-commands may have multiple commands with the same name, so a good practice is to assign a
// context value to each `ac_command_spec` which uniquely identifies the target command.
enum command_identifier {
    COMPRESSION   = 1,
    DECOMPRESSION = 2,
};

static struct ac_command_spec compression = {.help        = "Perform zlib compression.",
                                             .n_arguments = 1,
                                             .id          = COMPRESSION,
                                             .arguments =
                                                 (struct ac_argument_spec[]) {
                                                     {
                                                         .name = "FILE",
                                                         .help = "A path to the file to compress.",
                                                     },
                                                 },
                                             .n_options = 2,
                                             .options   = (struct ac_option_spec[]) {
                                                 {
                                                       .help      = "The compression level to use.",
                                                       .long_name = "level",
                                                       .has_short_name = true,
                                                       .short_name     = 'l',
                                                 },
                                                 {
                                                       .help = "Whether to print progress to stdout",
                                                       .long_name      = "progress",
                                                       .has_short_name = true,
                                                       .short_name     = 'p',
                                                       .is_flag        = true,
                                                 },
                                             }};

static struct ac_command_spec decompression = {
    .help        = "Perform zlib decompression.",
    .n_arguments = 1,
    .id          = DECOMPRESSION,
    .arguments =
        (struct ac_argument_spec[]) {
            {
                .name = "FILE",
                .help = "A path to the file to decompress.",
            },
        },
    .n_options = 2,
    .options   = (struct ac_option_spec[]) {
        {
              .help           = "The compression level to use.",
              .long_name      = "level",
              .has_short_name = true,
              .short_name     = 'l',
        },
        {
              .help           = "Whether to print progress to stdout",
              .long_name      = "progress",
              .has_short_name = true,
              .short_name     = 'p',
              .is_flag        = true,
        },
    }};

static struct ac_multi_command_spec multi_command = {
    .help          = "A zlib compress command line utility",
    .n_subcommands = 2,
    .subcommands =
        (struct ac_multi_command_subcommand[]) {
            {
                .name           = "compress",
                .type           = COMMAND_SINGLE,
                .single.command = &compression,
            },
            {
                .name           = "decompress",
                .type           = COMMAND_SINGLE,
                .single.command = &decompression,
            },
            // Nested subcommands can also be specfied here using a `COMMAND_PARENT` type, and a
            // .parent field in the struct.
        },
};

int main(int argc, char const *const argv[]) {
    if(argc <= 1) {
        // The @c ac_multicommand_help function is used for generating a help string. This returns
        // an owned string, so the caller is responsible outputting and freeing the buffer.
        printf("%s", ac_multicommand_help(&multi_command));
        return -1;
    }

    // Non performance restricted environments should validate that the multi-command specification
    // is valid using `ac_command_multivalidate`. Note, this will recursively validate all
    // subcommands.
    assert(ac_validate_multicommand(&multi_command).code == AC_ERROR_SUCCESS);

    // User input is parsed by calling `ac_parse_multicommand`. The `multi_command` specification
    // declares what valid user input is. The result of parsing will be populated in the provided
    // `args` structure.
    struct ac_command      args = {0};
    struct ac_status const result =
        ac_parse_multicommand(argc - 1, &argv[1], &multi_command, &args);
    if(!ac_status_is_success(result)) {
        // Woops, something went wrong. Call `ac_error_string` for a helpful error output.
        printf("%s", ac_error_string(result));
        return -1;
    }

    // Figure out which command we're doing using the context value.
    switch(args.command->id) {
        case COMPRESSION:
            printf("Doing compression!\n");
            break;
        case DECOMPRESSION:
            printf("Doing decompression!\n");
            break;
        default:
            assert(false);
    }

    struct ac_argument *path = ac_extract_argument(&args, "FILE");
    printf("Using file path: %s\n", path->value);

    // The presence of the `progress` option in the output structure indicate that the user set the
    // flag.
    bool progress = ac_extract_option(&args, "progress") != NULL;
    printf("tracking progress: %s\n", progress ? "YES" : "NO");

    // Default values are handled by the caller. If `level` is not in the output structure, then a
    // default may be chosen instead.
    struct ac_option *level_opt = ac_extract_option(&args, "level");
    char             *level     = (level_opt && level_opt->value) ? level_opt->value : "DEFAULT";
    printf("level set to: %s\n", level);

    // ... do something with the parsed command.

    return 0;
}
```