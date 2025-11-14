/**
 * @file args-c.h
 * @brief Single-header command-line argument parser for C
 */

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    /// @brief The maximum length of a string being parsed by args-c (in any context).
    MAX_STRING_LEN = 0x1000,
};
enum {
    /// @brief The maximum number of arguments that will be parsed by a command spec.
    MAX_NUM_ARGS = 0x100,
};
enum {
    /// @brief The size of a help message buffer.
    HELP_BUFFER_SZ = 0x1000,
};
enum {
    /// @brief The maximum number of options that will be parsed by a command spec.
    MAX_NUM_OPTIONS = 0x100,
};

enum ac_error_code {
    /// @brief Operation success.
    /// @par Context: None
    AC_ERROR_SUCCESS,

    /// @brief A parameter was invalid
    /// @par Context: None
    AC_ERROR_INVALID_PARAMETER,
    /// @brief Memory allocation failed.
    /// @par Context: None.
    AC_ERROR_MEMORY_ALLOC_FAILED,

    /// @brief A resolved option name was not found in the command specification.
    /// @par Context: char * of the option name used.
    AC_ERROR_OPTION_NAME_NOT_IN_SPEC,
    /// @brief An option name was expected, but a value was found instead.
    /// @par Context: None.
    AC_ERROR_OPTION_NAME_EXPECTED,
    /// @brief An option name is required in the command spec but not provided.
    /// @par Context: char * of the option name expected.
    AC_ERROR_OPTION_NAME_REQUIRED_IN_SPEC,
    /// @brief An option value was expected but something else was provided instead.
    /// @par Context: None,
    AC_ERROR_OPTION_VALUE_EXPECTED,
    /// @brief The number of options provided exceeded @c MAX_NUM_OPTIONS.
    /// @par Context: size_t of the number of options provided.
    AC_ERROR_OPTION_TOO_MANY,

    /// @brief A resolved command name was not found in the command specification.
    /// @par Context: char * of the command name used.
    AC_ERROR_COMMAND_NAME_NOT_IN_SPEC,
    /// @brief An command name is required, but not provided.
    /// @par Context: None,
    AC_ERROR_COMMAND_NAME_REQUIRED,
    /// @brief The provided command name is invalid.
    /// @par Context: char * of the invalid command name.
    AC_ERROR_COMMAND_NAME_INVALID,

    /// @brief Too many arguments were provided. This value can be configured with @c MAX_NUM_ARGS
    /// @par Context: size_t of the number of arguments used.
    AC_ERROR_ARGUMENT_MAX_EXCEEDED,
    /// @brief The number of arguments provided exceeded the number of arguments in the command
    /// spec.
    /// @par Context: size_t of the number of arguments provided.
    AC_ERROR_ARGUMENT_EXCEEDED_SPEC,
    /// @brief The number of arguments provided is smaller than the number of arguments in the
    /// command spec.
    /// @par Context: size_t of the number of arguments provided.
    AC_ERROR_ARGUMENT_EXPECTED_IN_SPEC,
};

struct ac_error {
    enum ac_error_code code;
    void              *context;
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

    // optional values to assign context or uniquely identify this command.
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
    char *help;

    size_t n_subcommands;
    struct ac_multicommand_subcommand {
        char                *name;
        enum ac_command_type type;
        union {
            struct {
                struct ac_command_spec *command;
            } terminal;

            struct {
                struct ac_multicommand_spec *subcommands;
            } parent;
        };
    } *subcommands;
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

static struct ac_error ac_parse_command(int const argc, char const *const *const argv,
                                        struct ac_command_spec const *const command,
                                        struct ac_command *const            args) {
    if(argv == NULL || command == NULL || args == NULL) {
        return {.code = AC_ERROR_INVALID_PARAMETER};
    }

    if(argc > MAX_NUM_ARGS) {
        return {.code = AC_ERROR_ARGUMENT_MAX_EXCEEDED, .context = (void *) (size_t) argc};
    }

    bzero(args, sizeof(*args));

    // We're going to continually reference the length of these strings, so
    // pre-populate an array for fast lookup.
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

        // Anything that's not a option name is implicitly an argument or value.
        if(!arguments_complete) {
            n_arguments++;
            tags[i] = TAG_ARGUMENT;
        } else {
            tags[i] = TAG_OPTION_VALUE;
        }
    }

    if(n_arguments > command->n_arguments) {
        return {.code = AC_ERROR_ARGUMENT_EXCEEDED_SPEC, .context = (void *) n_arguments};
    }
    if(n_arguments < command->n_arguments) {
        return {.code = AC_ERROR_ARGUMENT_EXPECTED_IN_SPEC, .context = (void *) n_arguments};
    }
    if(n_options > MAX_NUM_OPTIONS) {
        return {.code = AC_ERROR_OPTION_TOO_MANY, .context = (void *) n_options};
    }

    struct ac_argument *const arguments =
        n_arguments > 0 ? (struct ac_argument *) calloc(n_arguments, sizeof(*arguments)) : NULL;
    if(n_arguments != 0 && arguments == NULL) {
        return {.code = AC_ERROR_MEMORY_ALLOC_FAILED};
    }

    // Arguments are assigned in the order that they appear in the command.
    for(size_t i = 0; i < n_arguments; i++) {
        arguments->value    = strdup(argv[i]);
        arguments->argument = &command->arguments[i];
    }

    struct ac_option *options =
        n_options > 0 ? (struct ac_option *) calloc(n_options, sizeof(*options)) : NULL;
    if(n_options != 0 && options == NULL) {
        free(arguments);
        return {.code = AC_ERROR_MEMORY_ALLOC_FAILED};
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
                    return {.code = AC_ERROR_OPTION_VALUE_EXPECTED};
                }

                struct ac_option *const option = &options[options_idx];

                // Find the option that this maps to in the command spec.
                for(size_t j = 0; j < command->n_options; j++) {
                    struct ac_option_spec const *const option_spec = &command->options[j];
                    if(tags[i] == TAG_SHORT_OPTION) {
                        if(option_spec->has_short_name && option_spec->short_name == value[1]) {
                            option->option = option_spec;
                        }
                    } else if(tags[i] == TAG_LONG_OPTION) {
                        if(0 == strncmp(option_spec->long_name, &value[2], strlens[i] - 2)) {
                            option->option = option_spec;
                        }
                    } else {
                        assert(false);
                    }
                }

                if(option->option == NULL) {
                    cleanup();
                    return {.code = AC_ERROR_OPTION_NAME_NOT_IN_SPEC};
                }

                if(option->option->is_flag) {
                    options_idx++;
                } else {
                    expecting_value = true;
                }
                break;
            }
            case TAG_OPTION_VALUE: {
                if(!expecting_value) {
                    cleanup();
                    return {.code = AC_ERROR_OPTION_NAME_EXPECTED};
                }

                struct ac_option *const option = &options[options_idx];
                assert(options->option != NULL);

                option->value = strdup(value);

                expecting_value = false;
                options_idx++;
                break;
            }
        }
    }

    if(expecting_value) {
        cleanup();
        return {.code = AC_ERROR_OPTION_VALUE_EXPECTED};
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
                return {.code    = AC_ERROR_OPTION_NAME_REQUIRED_IN_SPEC,
                        .context = option_spec->long_name};
            }
        }
    }

    args->n_arguments = n_arguments;
    args->arguments   = arguments;
    args->n_options   = n_options;
    args->options     = options;
    args->command     = command;

    return {.code = AC_ERROR_SUCCESS};
}

static struct ac_error ac_parse_multicommand(int const argc, char const *const *const argv,
                                             struct ac_multicommand_spec const *const root,
                                             struct ac_command *const                 args) {
    if(argc == 0 || argv == NULL || root == NULL || args == NULL) {
        return {.code = AC_ERROR_INVALID_PARAMETER, .context = NULL};
    }

    if(argc > MAX_NUM_ARGS) {
        return {.code = AC_ERROR_ARGUMENT_MAX_EXCEEDED, .context = (void *) (size_t) argc};
    }

    // Figure out how many of the arguments are multi-command names.
    enum ac_error_code result          = AC_ERROR_SUCCESS;
    size_t             n_command_names = 0;
    for(size_t i = 0; i < argc; i++) {
        size_t namelen = strnlen(argv[i], MAX_STRING_LEN);
        if(namelen == 0 && n_command_names == 0) {
            // Empty string is always an invalid start.
            return {.code = AC_ERROR_COMMAND_NAME_INVALID, .context = (void *) argv[i]};
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
    for(; (i < n_command_names) && (command == NULL); i++) {
        char const *const curr_name = argv[i];
        bool              found     = false;

        for(size_t j = 0; j < curr_node->n_subcommands; j++) {
            if(0 == strncmp(curr_node->subcommands[j].name, curr_name, MAX_STRING_LEN)) {
                found = true;

                if(curr_node->subcommands[j].type == COMMAND_TERMINAL) {
                    command = curr_node->subcommands[j].terminal.command;
                    break;
                }

                // If this is the last iteration, then we expect the node to be terminal.
                if(i + 1 == n_command_names) {
                    return {.code = AC_ERROR_COMMAND_NAME_REQUIRED};
                }

                // Otherwise we've found a matching subcommand, progress to the next node.
                curr_node = curr_node->subcommands[j].parent.subcommands;
            }
        }

        if(!found) {
            return {.code = AC_ERROR_COMMAND_NAME_NOT_IN_SPEC, .context = (void *) curr_name};
        }
    }

    assert(command != NULL);

    return ac_parse_command(argc - i, &argv[i], command, args);
}

static size_t ac_strcpy_safe(char dst[], char const src[], size_t const offset,
                             size_t const dst_len) {
    size_t const src_len = strnlen(src, MAX_STRING_LEN);

    if(offset + src_len + 1 >= dst_len) {
        return 0;
    }

    memcpy(&dst[offset], src, src_len);
    dst[offset + src_len] = '\0';
    return src_len;
}

static char *ac_command_help(struct ac_command_spec const *const command) {
    char *const help = (char *) malloc(HELP_BUFFER_SZ);
    if(help == NULL) {
        return NULL;
    }

    size_t cursor = 0;
    cursor += ac_strcpy_safe(help, command->help, cursor, HELP_BUFFER_SZ);
    cursor += ac_strcpy_safe(help, "\n", cursor, HELP_BUFFER_SZ);

    if(command->n_arguments > 0) {
        assert(command->n_arguments <= MAX_NUM_ARGS);
        cursor += ac_strcpy_safe(help, "\nArguments:\n", cursor, HELP_BUFFER_SZ);

        size_t max_argument_name_len = 0;
        for(size_t i = 0; i < command->n_arguments; i++) {
            char const *const name = command->arguments[i].name;
            assert(name != NULL);

            size_t const name_len = strnlen(name, MAX_STRING_LEN);
            max_argument_name_len =
                name_len > max_argument_name_len ? name_len : max_argument_name_len;
        }

        for(size_t i = 0; i < command->n_arguments; i++) {
            char const *const name = command->arguments[i].name;
            assert(name != NULL);

            size_t const name_len = strnlen(name, MAX_STRING_LEN);
            cursor += ac_strcpy_safe(help, "  ", cursor, HELP_BUFFER_SZ);
            cursor += ac_strcpy_safe(help, name, cursor, HELP_BUFFER_SZ);
            for(size_t j = 0; j < (max_argument_name_len - name_len) + 1; j++) {
                cursor += ac_strcpy_safe(help, " ", cursor, HELP_BUFFER_SZ);
            }

            char *arghelp = command->arguments[i].help;
            if(arghelp != NULL) {
                cursor += ac_strcpy_safe(help, arghelp, cursor, HELP_BUFFER_SZ);
            }
            cursor += ac_strcpy_safe(help, "\n", cursor, HELP_BUFFER_SZ);
        }
    }

    if(command->n_options > 0) {
        assert(command->n_options <= MAX_NUM_OPTIONS);
        cursor += ac_strcpy_safe(help, "\nOptions:\n", cursor, HELP_BUFFER_SZ);

        bool   any_required        = false;
        size_t max_option_name_len = 0;
        for(size_t i = 0; i < command->n_options; i++) {
            struct ac_option_spec const *const option = &command->options[i];

            any_required |= option->required;

            char const *const name = option->long_name;
            assert(name != NULL);

            size_t const name_len = strnlen(name, MAX_STRING_LEN);
            max_option_name_len   = name_len > max_option_name_len ? name_len : max_option_name_len;
        }

        for(size_t i = 0; i < command->n_options; i++) {
            struct ac_option_spec const *const option = &command->options[i];

            cursor += ac_strcpy_safe(help, "  ", cursor, HELP_BUFFER_SZ);

            if(option->has_short_name) {
                cursor += ac_strcpy_safe(help, "-", cursor, HELP_BUFFER_SZ);
                help[cursor++] = option->short_name;
                cursor += ac_strcpy_safe(help, ", ", cursor, HELP_BUFFER_SZ);
            } else {
                cursor += ac_strcpy_safe(help, "    ", cursor, HELP_BUFFER_SZ);
            }

            char const *const long_name = option->long_name;
            assert(long_name != NULL);

            size_t const name_len = strnlen(long_name, MAX_STRING_LEN);
            cursor += ac_strcpy_safe(help, "--", cursor, HELP_BUFFER_SZ);
            cursor += ac_strcpy_safe(help, long_name, cursor, HELP_BUFFER_SZ);
            for(size_t j = 0; j < (max_option_name_len - name_len) + 1; j++) {
                cursor += ac_strcpy_safe(help, " ", cursor, HELP_BUFFER_SZ);
            }

            char const *const arghelp = option->help;
            if(arghelp != NULL) {
                cursor += ac_strcpy_safe(help, arghelp, cursor, HELP_BUFFER_SZ);
            }
            if(option->required) {
                cursor += ac_strcpy_safe(help, " (required)", cursor, HELP_BUFFER_SZ);
            }
            cursor += ac_strcpy_safe(help, "\n", cursor, HELP_BUFFER_SZ);
        }
    }

    return help;
}

static char *ac_multicommand_help(struct ac_multicommand_spec const *const command) {
    char *const help = (char *) malloc(HELP_BUFFER_SZ);
    if(help == NULL) {
        return NULL;
    }

    size_t cursor = 0;

    if(command->help != NULL) {
        cursor += ac_strcpy_safe(help, command->help, cursor, HELP_BUFFER_SZ);
        cursor += ac_strcpy_safe(help, "\n", cursor, HELP_BUFFER_SZ);
    }

    size_t max_command_name_len = 0;
    for(size_t i = 0; i < command->n_subcommands; i++) {
        assert(command->subcommands[i].name != NULL);
        size_t const name_len = strnlen(command->subcommands[i].name, MAX_STRING_LEN);
        max_command_name_len  = name_len > max_command_name_len ? name_len : max_command_name_len;
    }

    cursor += ac_strcpy_safe(help, "\nCommands:\n", cursor, HELP_BUFFER_SZ);
    for(size_t i = 0; i < command->n_subcommands; i++) {
        cursor += ac_strcpy_safe(help, "  ", cursor, HELP_BUFFER_SZ);
        cursor += ac_strcpy_safe(help, command->subcommands[i].name, cursor, HELP_BUFFER_SZ);

        size_t const name_len = strnlen(command->subcommands[i].name, MAX_STRING_LEN);
        for(size_t j = 0; j < (max_command_name_len - name_len) + 1; j++) {
            cursor += ac_strcpy_safe(help, " ", cursor, HELP_BUFFER_SZ);
        }

        switch(command->subcommands[i].type) {
            case COMMAND_TERMINAL: {
                if(command->subcommands[i].terminal.command->help) {
                    cursor += ac_strcpy_safe(help, command->subcommands[i].terminal.command->help,
                                             cursor, HELP_BUFFER_SZ);
                }
                break;
            }
            case COMMAND_PARENT: {
                if(command->subcommands[i].parent.subcommands->help) {
                    cursor += ac_strcpy_safe(help, command->subcommands[i].parent.subcommands->help,
                                             cursor, HELP_BUFFER_SZ);
                }
                break;
            }
        }
        cursor += ac_strcpy_safe(help, "\n", cursor, HELP_BUFFER_SZ);
    }

    return help;
}
