#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STRING_LEN 5120
#define MAX_NUM_ARGS   100

enum ac_error {
    AC_ERROR_SUCCESS,

    AC_ERROR_INVALID_PARAMETER,
    AC_ERROR_MEMORY_ALLOC_FAILED,
    AC_ERROR_EXCEEDED_MAX_NUM_ARGS,

    AC_ERROR_OPTION_NAME_NOT_IN_SPEC,
    AC_ERROR_OPTION_NAME_EXPECTED,
    AC_ERROR_OPTION_NAME_REQUIRED_IN_SPEC,
    AC_ERROR_OPTION_VALUE_EXPECTED,

    AC_ERROR_COMMAND_NAME_NOT_IN_SPEC,
    AC_ERROR_COMMAND_NAME_REQUIRED,

    AC_ERROR_ARGUMENT_EXCEEDED_SPEC,
    AC_ERROR_ARGUMENT_NOT_FOUND,
};

struct ac_argument_spec {
    char *name;
    char *help;
};

struct ac_option_spec {
    char *help;
    char *long_name;
    bool  has_short_name;
    char  short_name;
    bool  is_flag; // when false, a value is implicitly required.
    bool  required;
};

struct ac_command_spec {
    char *help;

    // optional values to assign context or uniqely identify this command.
    size_t id;
    void  *context;

    size_t                   n_arguments;
    struct ac_argument_spec *arguments;

    size_t                 n_options;
    struct ac_option_spec *options;
};

enum ac_command_type {
    COMMAND_TERMINAL,
    COMMAND_PARENT,
};

struct ac_multicommand_spec {
    size_t n_subcommands;
    struct ac_multicommand_subcommand {
        char                *name;
        enum ac_command_type type;
        union {
            struct {
                struct ac_command_spec *command;
            } terminal;

            struct {
                char                        *help;
                struct ac_multicommand_spec *subcommands;
            } parent;
        };
    } subcommands[];
};

struct ac_argument {
    struct ac_argument_spec const *argument;
    char                          *value;
};

struct ac_option {
    struct ac_option_spec const *option;
    char                        *value;
};

struct ac_command {
    struct ac_command_spec const *command;
    size_t                        n_arguments;
    struct ac_argument           *arguments;
    size_t                        n_options;
    struct ac_option             *options;
};

static bool ac_char_is_alpha(char const target) {
    return 'A' <= target && target <= 'z';
}

static bool ac_string_is_alpha(char const *const target, size_t const length) {
    for(size_t i = 0; i < strnlen(target, length); i++) {
        if(!ac_char_is_alpha(target[i])) {
            return false;
        }
    }

    return true;
}

static enum ac_error ac_parse_command(int const argc, char const *const *const argv,
                                      struct ac_command_spec const *const command,
                                      struct ac_command *const            args) {
    if(argv == NULL || command == NULL || args == NULL) {
        return AC_ERROR_INVALID_PARAMETER;
    }

    if(argc > MAX_NUM_ARGS) {
        return AC_ERROR_EXCEEDED_MAX_NUM_ARGS;
    }

    bzero(args, sizeof(*args));

    // We're going to continually reference the length of these strings, so
    // prepopulate an array for fast lookup.
    size_t strlens[MAX_NUM_ARGS] = {0};
    for(size_t i = 0; i < argc; i++) {
        strlens[i] = strnlen(argv[i], MAX_STRING_LEN);
    }

    enum tag {
        TAG_ARGUMENT,
        TAG_SHORT_OPTION,
        TAG_LONG_OPTION,
        TAG_OPTION_VALUE,
    };
    enum tag tags[MAX_NUM_ARGS];
    size_t   n_arguments        = 0;
    size_t   n_options          = 0;
    bool     arguments_complete = false;
    for(size_t i = 0; i < argc; i++) {
        // need to check for only alpha characters because e.g. '-1' is a valid
        // value.
        if(strlens[i] >= 3 && argv[i][0] == '-' && argv[i][1] == '-' &&
           ac_string_is_alpha(&argv[i][2], strlens[i])) {
            tags[i] = TAG_LONG_OPTION;
            n_options++;
            arguments_complete = true;
            continue;
        }

        if(strlens[i] == 2 && argv[i][0] == '-' && ac_char_is_alpha(argv[i][1])) {
            tags[i] = TAG_SHORT_OPTION;
            n_options++;
            arguments_complete = true;
            continue;
        }

        // Anything that's not a option name is implictly an argument or value.
        if(!arguments_complete) {
            n_arguments++;
            tags[i] = TAG_ARGUMENT;
        } else {
            tags[i] = TAG_OPTION_VALUE;
        }
    }

    if(n_arguments > command->n_arguments) {
        return AC_ERROR_ARGUMENT_EXCEEDED_SPEC;
    }
    if(n_arguments < command->n_arguments) {
        return AC_ERROR_ARGUMENT_NOT_FOUND;
    }

    struct ac_argument *const arguments =
        n_arguments > 0 ? (struct ac_argument *) calloc(n_arguments, sizeof(*arguments)) : NULL;
    if(n_arguments != 0 && arguments == NULL) {
        return AC_ERROR_MEMORY_ALLOC_FAILED;
    }

    // Arguments are assigned in the order that they appear in the command.
    for(size_t i = 0; i < n_arguments; i++) {
        arguments->value    = strdup(argv[i]);
        arguments->argument = &command->arguments[i];
    }

    struct ac_option *options = n_options > 0 ? (struct ac_option *) calloc(n_options, sizeof(*options)) : NULL;
    if(n_options != 0 && options == NULL) {
        free(arguments);
        return AC_ERROR_MEMORY_ALLOC_FAILED;
    }

#define cleanup()                                                                                  \
    free(arguments);                                                                               \
    free(options)

    size_t options_idx     = 0;
    bool   expecting_value = false;
    for(size_t i = n_arguments; i < argc; i++) {
        char const *const value = argv[i];
        switch(tags[i]) {
            case TAG_ARGUMENT: {
                assert(false);
            }
            case TAG_LONG_OPTION:
            case TAG_SHORT_OPTION: {
                if(expecting_value) {
                    cleanup();
                    return AC_ERROR_OPTION_VALUE_EXPECTED;
                }

                struct ac_option *const option = &options[options_idx];

                // Find the option that this maps to in the command spec.
                for(size_t i = 0; i < command->n_options; i++) {
                    struct ac_option_spec const *const option_spec = &command->options[i];
                    if(tags[i] == TAG_SHORT_OPTION) {
                        if(option_spec->has_short_name && option_spec->short_name == value[1]) {
                            option->option = option_spec;
                        }
                    } else {
                        if(strncmp(option_spec->long_name, &value[2], strlens[i] - 2)) {
                            option->option = option_spec;
                        }
                    }
                }

                if(option->option == NULL) {
                    cleanup();
                    return AC_ERROR_OPTION_NAME_NOT_IN_SPEC;
                }

                if(option->option->is_flag) {
                    options_idx++;
                } else {
                    expecting_value = true;
                }
            }
            case TAG_OPTION_VALUE: {
                if(!expecting_value) {
                    cleanup();
                    return AC_ERROR_OPTION_NAME_EXPECTED;
                }

                struct ac_option *const option = &options[options_idx];
                assert(options->option != NULL);

                option->value = strdup(value);

                expecting_value = false;
                options_idx++;
            }
        }
    }

    if(expecting_value) {
        cleanup();
        return AC_ERROR_OPTION_VALUE_EXPECTED;
    }

    // Make sure all the required options are present.
    for(size_t i = 0; i < command->n_options; i++) {
        struct ac_option_spec const *const option_spec = &command->options[i];
        if(option_spec->required) {
            bool found = false;
            for(size_t j = 0; j < n_options; j++) {
                if(options[j].option == option_spec) {
                    found = true;
                    break;
                }
            }

            if(!found) {
                cleanup();
                return AC_ERROR_OPTION_NAME_REQUIRED_IN_SPEC;
            }
        }
    }

    args->n_arguments = n_arguments;
    args->arguments   = arguments;
    args->n_options   = n_options;
    args->options     = options;
    args->command     = command;

    return AC_ERROR_SUCCESS;
}

static enum ac_error ac_parse_multicommand(int const argc, char const *const *const argv,
                                           struct ac_multicommand_spec const *const root,
                                           struct ac_command *const                 args) {
    if(argc == 0 || argv == NULL || root == NULL || args == NULL) {
        return AC_ERROR_INVALID_PARAMETER;
    }

    if(argc > MAX_NUM_ARGS) {
        return AC_ERROR_EXCEEDED_MAX_NUM_ARGS;
    }

    // Figure out how many of the arguments are multicommand names.
    enum ac_error result          = AC_ERROR_SUCCESS;
    size_t        n_command_names = 0;
    for(size_t i = 0; i < argc; i++) {
        size_t namelen = strnlen(argv[i], MAX_STRING_LEN);
        if(namelen == 0 && n_command_names) {
            // Empty string is always an invalid start.
            return AC_ERROR_OPTION_NAME_NOT_IN_SPEC;
        }
        if(namelen == 0 || argv[i][0] == '-') {
            break;
        }
        n_command_names++;
    }

    // Traverse the command tree to resolve the command.
    struct ac_multicommand_spec const *curr_node = root;
    struct ac_command_spec const      *command   = NULL;
    size_t                             i         = 0;
    for(; i < n_command_names; i++) {
        char const *const curr_name = argv[i];
        bool              found     = false;

        for(size_t j = 0; j < curr_node->n_subcommands; j++) {
            if(0 == strncmp(curr_node->subcommands[j].name, curr_name, MAX_STRING_LEN) == 0) {
                found = true;

                if(curr_node->subcommands[j].type == COMMAND_TERMINAL) {
                    command = curr_node->subcommands[j].terminal.command;
                    break;
                }

                // If this is the last iteration, then we expect the node to be terminal.
                if(i + 1 == n_command_names) {
                    return AC_ERROR_COMMAND_NAME_REQUIRED;
                }

                // Otherwise we've found a matching subcommand, progress to the next node.
                curr_node = curr_node->subcommands[j].parent.subcommands;
            }
        }

        if(!found) {
            return AC_ERROR_COMMAND_NAME_NOT_IN_SPEC;
        }
    }

    assert(command != NULL);

    return ac_parse_command(argc - i, &argv[i], command, args);
}