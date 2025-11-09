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
    AC_ERROR_COMMAND_NAME_INVALID,
    AC_ERROR_TOO_MANY_COMMAND_NAMES,
    AC_ERROR_UNKNOWN_COMMAND_NAME,
    AC_ERROR_EXPECTED_MORE_COMMAND_NAMES,
    AC_ERROR_EXCEEDED_MAX_NUM_ARGS,
    AC_ERROR_EXPECTED_COMMAND_NAME,
    AC_ERROR_EXPECTED_VALUE,
    AC_ERROR_EXPECTED_NO_VALUE,
    AC_ERROR_INVALID_INTEGER,
    AC_ERROR_MISSING_REQUIRED_ARGUMENT,
};

enum ac_argument_type {
    ARGUMENT_TYPE_NONE,
    ARGUMENT_TYPE_STRING,
    ARGUMENT_TYPE_UNSIGNED_INTEGER,
    ARGUMENT_TYPE_SIGNED_INTEGER,
};

struct ac_argument_spec {
    char                 *long_name;
    bool                  has_short_name;
    char                  short_name;
    char                 *help;
    bool                  required;
    enum ac_argument_type type;
};

struct ac_command_spec {
    char                    *help;

    // optional values to assign context or uniqely identify this command.
    size_t id;
    void  *context;

    size_t                   n_arguments;
    struct ac_argument_spec arguments[];
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
    union {
        char    *string;
        uint64_t unsigned_integer;
        int64_t  signed_integer;
    } value;
};

struct ac_arguments {
    struct ac_command_spec const *command;
    size_t                        n_arguments;
    struct ac_argument            arguments[];
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
                                      struct ac_arguments const **const   args) {
    if(argv == NULL || command == NULL || args == NULL) {
        return AC_ERROR_INVALID_PARAMETER;
    }

    if(argc > MAX_NUM_ARGS) {
        return AC_ERROR_EXCEEDED_MAX_NUM_ARGS;
    }

    // We're going to continually reference the length of these strings, so
    // prepopulate an array for fast lookup.
    size_t strlens[MAX_NUM_ARGS] = {0};
    for(size_t i = 0; i < argc; i++) {
        strlens[i] = strnlen(argv[i], MAX_STRING_LEN);
    }

    enum tag {
        TAG_SHORT_ARG,
        TAG_LONG_ARG,
        TAG_VALUE,
    };
    enum tag tags[MAX_NUM_ARGS];
    size_t   n_arguments = 0;
    for(size_t i = 0; i < argc; i++) {
        if(strlens[i] < 2) {
            tags[i] = TAG_VALUE;
            n_arguments++;
            continue;
        }

        // need to check for only alpha characters because e.g. '-1' is a valid
        // value.
        if(strlens[i] >= 3 && argv[i][0] == '-' && argv[i][1] == '-' &&
           ac_string_is_alpha(&argv[i][2], strlens[i])) {
            tags[i] = TAG_LONG_ARG;
            n_arguments++;
            continue;
        }

        if(strlens[i] == 2 && argv[i][0] == '-' && ac_char_is_alpha(argv[i][1])) {
            tags[i] = TAG_SHORT_ARG;
            n_arguments++;
            continue;
        }

        // Anything that's not a command is implictly a value.
        tags[i] = TAG_VALUE;
    }

    // Find the respective argument spec value and populate the output structure.
    struct ac_arguments *const arguments =
        (struct ac_arguments *) malloc(sizeof(*args) + sizeof(struct ac_argument) * n_arguments);
    bool   expecting_value = false;
    size_t args_idx        = 0;
    for(size_t i = 0; i < argc; i++) {
        char const *const value = argv[i];
        switch(tags[i]) {
            case TAG_LONG_ARG:
            case TAG_SHORT_ARG: {
                if(expecting_value) {
                    free(arguments);
                    return AC_ERROR_EXPECTED_VALUE;
                }

                struct ac_argument *const arg = &arguments->arguments[args_idx];

                for(size_t i = 0; i < command->n_arguments; i++) {
                    struct ac_argument_spec const *const arg_spec = &command->arguments[i];
                    if(tags[i] == TAG_SHORT_ARG) {
                        if(arg_spec->has_short_name && arg_spec->short_name == value[1]) {
                            arg->argument = arg_spec;
                        }
                    } else {
                        if(strncmp(arg_spec->long_name, &value[2], strlens[i] - 2)) {
                            arg->argument = arg_spec;
                        }
                    }
                }

                if(arg->argument == NULL) {
                    free(arguments);
                    return AC_ERROR_COMMAND_NAME_INVALID;
                }

                if(arg->argument->type == ARGUMENT_TYPE_NONE) {
                    args_idx++;
                } else {
                    expecting_value = true;
                }
            }
            case TAG_VALUE: {
                if(!expecting_value) {
                    free(arguments);
                    return AC_ERROR_EXPECTED_COMMAND_NAME;
                }

                struct ac_argument *const arg = &arguments->arguments[args_idx];
                switch(arg->argument->type) {
                    case ARGUMENT_TYPE_NONE: {
                        free(arguments);
                        return AC_ERROR_EXPECTED_NO_VALUE;
                    }
                    case ARGUMENT_TYPE_STRING: {
                        arg->value.string = strdup(value);
                        break;
                    }
                    case ARGUMENT_TYPE_SIGNED_INTEGER:
                    case ARGUMENT_TYPE_UNSIGNED_INTEGER: {
                        int         base        = 10;
                        char const *start       = value;
                        bool        is_negative = false;
                        if(strlens[i] > 2 && value[0] == '0' && value[1] == 'x') {
                            base  = 0x10;
                            start = &value[2];
                        }
                        if(arg->argument->type == ARGUMENT_TYPE_SIGNED_INTEGER && value[0] == '-') {
                            if(strlens[i] > 3 && value[1] == '0' && value[2] == 'x') {
                                base  = 0x10;
                                start = &value[3];
                            } else {
                                start = &value[1];
                            }
                            is_negative = true;
                        }

                        char        *end    = NULL;
                        size_t const number = strtol(&value[2], &end, base);
                        if(end != value + strlens[i]) {
                            free(arguments);
                            return AC_ERROR_INVALID_INTEGER;
                        }

                        if(arg->argument->type == ARGUMENT_TYPE_UNSIGNED_INTEGER) {
                            arg->value.unsigned_integer = number;
                        } else {
                            arg->value.signed_integer = is_negative ? number : -number;
                        }

                        break;
                    }
                }

                expecting_value = false;
                args_idx++;
            }
        }
    }

    if(expecting_value) {
        free(arguments);
        return AC_ERROR_EXPECTED_VALUE;
    }

    // Make sure all the required arguments are present.
    for(size_t i = 0; i < command->n_arguments; i++) {
        struct ac_argument_spec const *const arg_spec = &command->arguments[i];
        if(arg_spec->required) {
            bool found = false;
            for(size_t j = 0; j < arguments->n_arguments; j++) {
                if(arguments->arguments[j].argument == arg_spec) {
                    found = true;
                    break;
                }
            }

            if(!found) {
                free(arguments);
                return AC_ERROR_MISSING_REQUIRED_ARGUMENT;
            }
        }
    }

    *args = arguments;

    return AC_ERROR_SUCCESS;
}

static enum ac_error ac_parse_multicommand(int const argc, char const *const *const argv,
                                           struct ac_multicommand_spec const *const root,
                                           struct ac_arguments const **const        args) {
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
            return AC_ERROR_COMMAND_NAME_INVALID;
        }
        if(namelen == 0 || argv[i][0] == '-') {
            break;
        }
        n_command_names++;
    }

    // Traverse the command tree to resolve the command.
    struct ac_multicommand_spec const *curr_node = root;
    struct ac_command_spec const      *command   = NULL;
    for(size_t i = 0; i < n_command_names; i++) {
        char const *const curr_name = argv[i];
        bool              found     = false;

        for(size_t j = 0; j < curr_node->n_subcommands; j++) {
            if(strncmp(curr_node->subcommands[j].name, curr_name, MAX_STRING_LEN) == 0) {
                found = true;

                // If this is the last iteration, then we expect the node to be terminal.
                if(i + 1 == n_command_names) {
                    if(curr_node->subcommands[j].type != COMMAND_TERMINAL) {
                        return AC_ERROR_EXPECTED_MORE_COMMAND_NAMES;
                    }

                    command = curr_node->subcommands[j].terminal.command;
                }
                // Otherwise we've found a matching subcommand, progress to the next node.
                else {
                    if(curr_node->subcommands[j].type != COMMAND_PARENT) {
                        return AC_ERROR_TOO_MANY_COMMAND_NAMES;
                    }

                    curr_node = curr_node->subcommands[j].parent.subcommands;
                }

                break;
            }
        }

        if(!found) {
            return AC_ERROR_UNKNOWN_COMMAND_NAME;
        }
    }

    if(command == NULL) {
        return AC_ERROR_EXPECTED_MORE_COMMAND_NAMES;
    }

    return ac_parse_command(argc - n_command_names, &argv[n_command_names], command, args);
}