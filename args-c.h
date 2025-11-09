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
    AC_ERROR_EXPECTED_MORE_COMMANDS,
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

enum ac_command_type {
    COMMAND_TERMINAL,
    COMMAND_PARENT,
};

struct ac_command_spec {
    char *name;
    char *help;

    size_t id;
    void  *context;

    enum ac_command_type type;

    union {
        struct {
            size_t                   n_arguments;
            struct ac_argument_spec *arguments;
        } terminal;

        struct {
            size_t                  n_subcommands;
            struct ac_command_spec *subcommands;
        } parent;
    };
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

static struct ac_argument_spec const *
find_argument_with_short_name(struct ac_command_spec const *const node, char const short_name) {
    assert(node->type == COMMAND_TERMINAL);
    for(size_t i = 0; i < node->terminal.n_arguments; i++) {
        struct ac_argument_spec const *const arg = &node->terminal.arguments[i];
        if(arg->has_short_name && arg->short_name == short_name) {
            return arg;
        }
    }

    return NULL;
}

static struct ac_argument_spec const *
find_argument_with_long_name(struct ac_command_spec const *const node,
                             char const *const                   long_name) {
    assert(node->type == COMMAND_TERMINAL);
    for(size_t i = 0; i < node->terminal.n_arguments; i++) {
        struct ac_argument_spec const *const arg = &node->terminal.arguments[i];
        if(arg->long_name == long_name) {
            return arg;
        }
    }

    return NULL;
}

static enum ac_error ac_parse_argv(int const argc, char const *const *const argv,
                                   struct ac_command_spec const *const root,
                                   struct ac_arguments const **const   args) {
    if(argc == 0 || argv == NULL || root == NULL || args == NULL) {
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

    // Figure out how many of the arguments are 'command' values.
    enum ac_error result           = AC_ERROR_SUCCESS;
    size_t        n_command_tokens = 0;
    for(size_t i = 0; i < argc; i++) {
        if(strlens[i] == 0 && n_command_tokens) {
            // Empty string is always an invalid start.
            return AC_ERROR_COMMAND_NAME_INVALID;
        }
        if(strlens[i] == 0 || argv[i][0] == '-') {
            break;
        }
        n_command_tokens++;
    }

    // Traverse the command tree to resolve the command.
    struct ac_command_spec const *curr_node = root;
    for(size_t i = 0; i < n_command_tokens - 1; i++) {
        char const *const curr_token = argv[i];
        if(curr_node->type == COMMAND_TERMINAL) {
            // This is a terminal node, so an extra command name is invalid here.
            return AC_ERROR_TOO_MANY_COMMAND_NAMES;
        }

        bool found = false;
        for(size_t j = 0; j < curr_node->parent.n_subcommands; j++) {
            if(strncmp(curr_node->parent.subcommands[j].name, curr_token, MAX_STRING_LEN) == 0) {
                // Found a matching subcommand, progress to the next node.
                curr_node = &curr_node->parent.subcommands[j];
                found     = true;
                break;
            }
        }

        if(!found) {
            return AC_ERROR_UNKNOWN_COMMAND_NAME;
        }
    }

    // We've either run out of tokens or hit an argument token, either way the
    // curr_node should be terminal.
    if(curr_node->type != COMMAND_TERMINAL) {
        return AC_ERROR_EXPECTED_MORE_COMMANDS;
    }

    // Fast path: the command has no arguments so our job here is done.
    if(n_command_tokens == argc) {
        struct ac_arguments *const arguments = (struct ac_arguments *) malloc(sizeof(*args));
        if(arguments == NULL) {
            return AC_ERROR_MEMORY_ALLOC_FAILED;
        }

        arguments->command     = curr_node;
        arguments->n_arguments = 0;
        *args                  = arguments;

        return AC_ERROR_SUCCESS;
    }

    // Tag the remaining arguments as either 'long' argument, 'short' argument, or
    // 'value'.
    enum tag {
        TAG_SHORT_ARG,
        TAG_LONG_ARG,
        TAG_VALUE,
    };
    enum tag tags[MAX_NUM_ARGS];
    size_t   n_arguments = 0;
    for(size_t i = n_command_tokens; i < argc; i++) {
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
    for(size_t i = n_command_tokens; i < argc; i++) {
        char const *const value = argv[i];
        switch(tags[i]) {
            case TAG_LONG_ARG:
            case TAG_SHORT_ARG: {
                if(expecting_value) {
                    free(arguments);
                    return AC_ERROR_EXPECTED_VALUE;
                }

                struct ac_argument *const arg = &arguments->arguments[args_idx];

                for(size_t i = 0; i < curr_node->terminal.n_arguments; i++) {
                    struct ac_argument_spec const *const arg_spec =
                        &curr_node->terminal.arguments[i];
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
    for(size_t i = 0; i < curr_node->terminal.n_arguments; i++) {
        struct ac_argument_spec const *const arg_spec = &curr_node->terminal.arguments[i];
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