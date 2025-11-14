
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
