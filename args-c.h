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

enum ac_status_code {
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
    /// @par Context: char * of the incorrect value.
    AC_ERROR_OPTION_NAME_EXPECTED,
    /// @brief An option name is required in the command spec but not provided.
    /// @par Context: char * of the option name expected.
    AC_ERROR_OPTION_NAME_REQUIRED_IN_SPEC,
    /// @brief An option value was expected but something else was provided instead.
    /// @par Context: char * of the option name for which a value was expected,
    AC_ERROR_OPTION_VALUE_EXPECTED,
    /// @brief The number of options provided exceeded @c MAX_NUM_OPTIONS.
    /// @par Context: size_t of the number of options provided.
    AC_ERROR_OPTION_TOO_MANY,
    /// @brief An option specification was provided that didn't contain a long name for the option.
    /// @par Context: size_t of the index of the bad option in the `ac_command_spec`.
    AC_ERROR_OPTION_SPEC_NEEDS_NAME,
    /// @brief An option specification was provided that had an invalid long name.
    /// @par Context: size_t of the index of the bad option in the `ac_command_spec`.
    AC_ERROR_OPTION_LONG_NAME_INVALID,
    /// @brief An option specification was provided that had an invalid short name.
    /// @par Context: size_t of the index of the bad option in the `ac_command_spec`.
    AC_ERROR_OPTION_SHORT_NAME_INVALID,
    /// @brief An option specification was that had both `is_flag` and `required` set.
    /// @par Context: size_t of the index of the bad option in the `ac_command_spec`.
    AC_ERROR_OPTION_FLAG_AND_REQUIRED,

    /// @brief A resolved command name was not found in the command specification.
    /// @par Context: char * of the command name used.
    AC_ERROR_COMMAND_NAME_NOT_IN_SPEC,
    /// @brief An command name is required, but not provided.
    /// @par Context: char * of the command name that requires another command name.
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
    /// @brief An argument specification was provided that didn't contain a name for the argument.
    /// @par Context: size_t of the index of the bad argument in the `ac_command_spec`.
    AC_ERROR_ARGUMENT_SPEC_NEEDS_NAME,

    /// @brief The provided multi-command contains a subcommand without a name..
    /// @par Context: size_t of the index of the invalid subcommand.
    AC_ERROR_MULTICOMMAND_NEEDS_NAME,
};

/// @brief Describes the result of an args-c operation.
struct ac_status {
    /// @brief A code that indicates the result of an operation.
    enum ac_status_code code;

    /// @brief The command that was being processed when the error occurred, if at all.
    struct ac_command_spec const *single;

    /// @brief The multi-command that was being processed when the error occurred, if at all.
    struct ac_multi_command_spec const *multi;

    /// @brief A status code specific context value that can be used to debug the cause of the
    /// specified status code.
    /// @par The context values are described by documentation in the @c ac_status_code enum.
    void *context;
};

/// @brief A convenience function for determining if the provided `status` indicates a successful
/// operation.
static bool ac_status_is_success(struct ac_status const status) {
    return status.code == AC_ERROR_SUCCESS;
}

/// @brief Encapsulates an argument specification.
/// @par In args-c, an 'argument' is a required positional value. For example, the file argument to
/// unix "cat <file>".
/// @par The caller provides argument specifications as part of the @c ac_command_spec
/// structure which is a parameter to @c ac_command_parse.
struct ac_argument_spec {
    /// @brief The name of this argument. This name only has semantic value, and won't appear user's
    /// command.
    char *name;
    /// @brief A help string that will appear in the @c ac_command_help output.
    char *help;
};

/// @brief Encapsulates an option specification.
/// @par In args-c, an 'option' is a named value. For example, the input argument to unix "sed -i
/// <file>".
/// @par Options must have a 'long name' which will be used on the command line as --name.
/// Optionally, a short name may also be specified and the user can use either interchangeably.
struct ac_option_spec {
    /// @brief A help string that will appear in the @c ac_command_help output.
    char *help;
    /// @brief The long name of this option.
    char *long_name;
    /// @brief @c true when this option has a short name.
    bool has_short_name;
    /// @brief The short name of this option when @c has_short_name is @c true.
    char short_name;
    /// @brief Whether this option is a 'flag' value. Non-flag options expect a value after their
    /// --name in a command.
    /// @par When @c false, a value is implicitly required.
    bool is_flag;
    /// @brief Whether this option is mandatory in the command.
    bool required;
};

/// @brief Describes whether a command in a multi-command points to another multi-command or just a
/// regular command.
enum ac_command_type {
    /// @brief An @c ac_command_spec value.
    COMMAND_SINGLE,
    /// @brief An @c ac_multi_command_spec value.
    COMMAND_MULTI,
};

/// @brief Encapsulates a command specification.
/// @par This structure is passed to @c ac_parse_command to describe the structure of the command to
/// be parsed.
struct ac_command_spec {
    /// @brief A help string that will appear in the @c ac_command_help output.
    char *help;

    /// @brief An optional value to assign context or uniquely identify this command.
    size_t id;
    /// @brief An optional value to assign context or uniquely identify this command.
    void *context;

    /// @brief The number of arguments that this command expects.
    size_t n_arguments;
    /// @brief An array of arguments for this command. Must contain exactly @c n_arguments elements.
    struct ac_argument_spec *arguments;

    /// @brief The number of options that this command expects.
    size_t n_options;
    /// @brief An array of options for this command. Must contain exactly @c n_options elements.
    struct ac_option_spec *options;
};

/// @brief Encapsulates a multi-command specification.
/// @par This structure is passed to @c ac_parse_multicommand to describe the structure of the
/// multi-command to be parsed.
struct ac_multi_command_spec {
    /// @brief A help string that will appear in the @c ac_multicommand_help output.
    char *help;

    /// @brief The number of commands that this multi-command encapsulates.
    size_t n_subcommands;
    /// @brief An array of subcommand for this command. Must contain exactly @c n_subcommands
    /// elements.
    struct ac_multi_command_subcommand {
        /// @brief The name of this multi-command. This is the name provided by the user to invoke
        /// this subcommand.
        char *name;
        /// @brief Indicates whether this sub-command is another multi-command or just a command.
        enum ac_command_type type;
        /// @brief The multi-command or command itself.
        union {
            struct {
                struct ac_command_spec *command;
            } single;

            struct {
                struct ac_multi_command_spec *subcommands;
            } multi;
        };
    } *subcommands;
};

/// @brief An output argument returned from parsing a user command.
struct ac_argument {
    /// @brief A pointer to the argument specification that made this argument parseable.
    struct ac_argument_spec const *argument;
    /// @brief The value provided for this argument.
    char *value;
};

/// @brief An output option returned from parsing a user command.
struct ac_option {
    /// @brief A pointer to the option specification that made this argument parseable.
    struct ac_option_spec const *option;
    /// @brief The value provided to the this option, only when @c option->is_flag is @c false.
    char *value;
};

/// @brief An output command returned from parsing a user command.
struct ac_command {
    /// @brief A pointer to the command specification used to parse the user input.
    struct ac_command_spec const *command;
    /// @brief The number of arguments parsed from the user input.
    size_t n_arguments;
    /// @brief An array of arguments with @c n_arguments elements.
    struct ac_argument *arguments;
    /// @brief The number of options parsed from the user input.
    size_t n_options;
    /// @brief An array of options with @c n_options elements.
    struct ac_option *options;
};

inline static bool _ac_char_is_alpha(char const target) {
    return 'A' <= target && target <= 'z';
}

inline static bool _ac_string_is_alpha(char const *const target, size_t const length) {
    for(size_t i = 0; i < strnlen(target, length); i++) {
        if(!_ac_char_is_alpha(target[i])) {
            return false;
        }
    }

    return true;
}

/// @brief Parse user input using the provided @p command specification.
/// @param argc The number of elements in @p argv
/// @param argv The user input to be parsed. Must contain exactly @p argc elements.
/// @note When parsing arguments from `int main(int argc, char **argv)`, the caller will
/// typically want to cut the executable path (element 0) from the @p argv array when calling this
/// function.
/// @param command The command specification which describes how to parse @p argv .
/// @param args An output structure that contains the parsed values when the return code is @c
/// AC_ERROR_SUCCESS.
/// @result @c AC_ERROR_SUCCESS when the user input is successfully parsed.
static struct ac_status ac_parse_command(int const argc, char const *const *const argv,
                                         struct ac_command_spec const *const command,
                                         struct ac_command *const            args) {
#define AC_STATUS(...) (struct ac_status){.single = command, ##__VA_ARGS__};

    if(argv == NULL || command == NULL || args == NULL) {
        return AC_STATUS(.code = AC_ERROR_INVALID_PARAMETER);
    }

    if(argc > MAX_NUM_ARGS) {
        return AC_STATUS(.code = AC_ERROR_ARGUMENT_MAX_EXCEEDED, .context = (void *) (size_t) argc);
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
           _ac_string_is_alpha(&argv[i][2], strlens[i])) {
            tags[i] = TAG_LONG_OPTION;
            n_options++;
            arguments_complete = true;
            continue;
        }

        if(strlens[i] == 2 && argv[i][0] == '-' && _ac_char_is_alpha(argv[i][1])) {
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
        return AC_STATUS(.code = AC_ERROR_ARGUMENT_EXCEEDED_SPEC, .context = (void *) n_arguments);
    }
    if(n_arguments < command->n_arguments) {
        return AC_STATUS(.code    = AC_ERROR_ARGUMENT_EXPECTED_IN_SPEC,
                         .context = (void *) n_arguments);
    }
    if(n_options > MAX_NUM_OPTIONS) {
        return AC_STATUS(.code = AC_ERROR_OPTION_TOO_MANY, .context = (void *) n_options);
    }

    struct ac_argument *const arguments =
        n_arguments > 0 ? (struct ac_argument *) calloc(n_arguments, sizeof(*arguments)) : NULL;
    if(n_arguments != 0 && arguments == NULL) {
        return AC_STATUS(.code = AC_ERROR_MEMORY_ALLOC_FAILED);
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
        return AC_STATUS(.code = AC_ERROR_MEMORY_ALLOC_FAILED);
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
                    return AC_STATUS(.code    = AC_ERROR_OPTION_VALUE_EXPECTED,
                                     .context = (char *) options[options_idx].option->long_name);
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
                    return AC_STATUS(.code    = AC_ERROR_OPTION_NAME_NOT_IN_SPEC,
                                     .context = (void *) value);
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
                    return AC_STATUS(.code    = AC_ERROR_OPTION_NAME_EXPECTED,
                                     .context = (char *) value);
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
        return AC_STATUS(.code    = AC_ERROR_OPTION_VALUE_EXPECTED,
                         .context = (char *) options[options_idx].option->long_name);
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
                return AC_STATUS(.code    = AC_ERROR_OPTION_NAME_REQUIRED_IN_SPEC,
                                 .context = option_spec->long_name);
            }
        }
    }

    args->n_arguments = n_arguments;
    args->arguments   = arguments;
    args->n_options   = n_options;
    args->options     = options;
    args->command     = command;

    return AC_STATUS(.code = AC_ERROR_SUCCESS);
#undef cleanup
#undef AC_STATUS
}

/// @brief Parse user input using the provided @p root multi-command specification.
/// @param argc The number of elements in @p argv
/// @param argv The user input to be parsed. Must contain exactly @p argc elements.
/// @note When parsing arguments from @c 'int main(int argc, char **argv)', the caller will
/// typically want to cut the executable path (element 0) from the @p argv array when calling this
/// function.
/// @param root The multi-command specification which describes how to parse @p argv .
/// @param args An output structure that contains the parsed values when the return code is @c
/// AC_ERROR_SUCCESS.
/// @result @c AC_ERROR_SUCCESS when the user input is successfully parsed.
static struct ac_status ac_parse_multicommand(int const argc, char const *const *const argv,
                                              struct ac_multi_command_spec const *const root,
                                              struct ac_command *const                  args) {
    if(argc == 0 || argv == NULL || root == NULL || args == NULL) {
        return (struct ac_status) {.code = AC_ERROR_INVALID_PARAMETER, .multi = root};
    }

    if(argc > MAX_NUM_ARGS) {
        return (struct ac_status) {.code    = AC_ERROR_ARGUMENT_MAX_EXCEEDED,
                                   .context = (void *) (size_t) argc,
                                   .multi   = root};
    }

    // Figure out how many of the arguments are multi-command names.
    enum ac_status_code result          = AC_ERROR_SUCCESS;
    size_t              n_command_names = 0;
    for(size_t i = 0; i < argc; i++) {
        size_t namelen = strnlen(argv[i], MAX_STRING_LEN);
        if(namelen == 0 && n_command_names == 0) {
            // Empty string is always an invalid start.
            return (struct ac_status) {
                .code = AC_ERROR_COMMAND_NAME_INVALID, .context = (void *) argv[i], .multi = root};
        }
        if(namelen == 0 || argv[i][0] == '-') {
            break;
        }
        n_command_names++;
    }

    // Traverse the command tree to resolve the command.
    struct ac_multi_command_spec const *curr_node = root;
    struct ac_command_spec const       *command   = NULL;
    size_t                              i         = 0;
    for(; (i < n_command_names) && (command == NULL); i++) {
        char const *const curr_name = argv[i];
        bool              found     = false;

        for(size_t j = 0; j < curr_node->n_subcommands; j++) {
            if(0 == strncmp(curr_node->subcommands[j].name, curr_name, MAX_STRING_LEN)) {
                found = true;

                if(curr_node->subcommands[j].type == COMMAND_SINGLE) {
                    command = curr_node->subcommands[j].single.command;
                    break;
                }

                // If this is the last iteration, then we expect the node to be single.
                if(i + 1 == n_command_names) {
                    return (struct ac_status) {.code    = AC_ERROR_COMMAND_NAME_REQUIRED,
                                               .multi   = curr_node,
                                               .context = (char *) curr_name};
                }

                // Otherwise we've found a matching subcommand, progress to the next node.
                curr_node = curr_node->subcommands[j].multi.subcommands;
            }
        }

        if(!found) {
            return (struct ac_status) {.code    = AC_ERROR_COMMAND_NAME_NOT_IN_SPEC,
                                       .context = (void *) curr_name,
                                       .multi   = curr_node};
        }
    }

    assert(command != NULL);

    return ac_parse_command(argc - i, &argv[i], command, args);
}

static size_t _ac_strcpy_safe(char dst[], char const src[], size_t const offset,
                              size_t const dst_len) {
    size_t const src_len = strnlen(src, MAX_STRING_LEN);

    if(offset + src_len + 1 >= dst_len) {
        return 0;
    }

    memcpy(&dst[offset], src, src_len);
    dst[offset + src_len] = '\0';
    return src_len;
}

/// @brief Generate a help text string for the given @p command specification
/// @param command The command to generate a help string for.
/// @result A help string if successful, otherwise @c NULL.
static char *ac_command_help(struct ac_command_spec const *const command) {
    char *const help = (char *) malloc(HELP_BUFFER_SZ);
    if(help == NULL) {
        return NULL;
    }

    size_t cursor = 0;
    cursor += _ac_strcpy_safe(help, command->help, cursor, HELP_BUFFER_SZ);
    cursor += _ac_strcpy_safe(help, "\n", cursor, HELP_BUFFER_SZ);

    if(command->n_arguments > 0) {
        assert(command->n_arguments <= MAX_NUM_ARGS);
        cursor += _ac_strcpy_safe(help, "\nArguments:\n", cursor, HELP_BUFFER_SZ);

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
            cursor += _ac_strcpy_safe(help, "  ", cursor, HELP_BUFFER_SZ);
            cursor += _ac_strcpy_safe(help, name, cursor, HELP_BUFFER_SZ);
            for(size_t j = 0; j < (max_argument_name_len - name_len) + 1; j++) {
                cursor += _ac_strcpy_safe(help, " ", cursor, HELP_BUFFER_SZ);
            }

            char *arghelp = command->arguments[i].help;
            if(arghelp != NULL) {
                cursor += _ac_strcpy_safe(help, arghelp, cursor, HELP_BUFFER_SZ);
            }
            cursor += _ac_strcpy_safe(help, "\n", cursor, HELP_BUFFER_SZ);
        }
    }

    if(command->n_options > 0) {
        assert(command->n_options <= MAX_NUM_OPTIONS);
        cursor += _ac_strcpy_safe(help, "\nOptions:\n", cursor, HELP_BUFFER_SZ);

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

            cursor += _ac_strcpy_safe(help, "  ", cursor, HELP_BUFFER_SZ);

            if(option->has_short_name) {
                cursor += _ac_strcpy_safe(help, "-", cursor, HELP_BUFFER_SZ);
                help[cursor++] = option->short_name;
                cursor += _ac_strcpy_safe(help, ", ", cursor, HELP_BUFFER_SZ);
            } else {
                cursor += _ac_strcpy_safe(help, "    ", cursor, HELP_BUFFER_SZ);
            }

            char const *const long_name = option->long_name;
            assert(long_name != NULL);

            size_t const name_len = strnlen(long_name, MAX_STRING_LEN);
            cursor += _ac_strcpy_safe(help, "--", cursor, HELP_BUFFER_SZ);
            cursor += _ac_strcpy_safe(help, long_name, cursor, HELP_BUFFER_SZ);
            for(size_t j = 0; j < (max_option_name_len - name_len) + 1; j++) {
                cursor += _ac_strcpy_safe(help, " ", cursor, HELP_BUFFER_SZ);
            }

            char const *const arghelp = option->help;
            if(arghelp != NULL) {
                cursor += _ac_strcpy_safe(help, arghelp, cursor, HELP_BUFFER_SZ);
            }
            if(option->required) {
                cursor += _ac_strcpy_safe(help, " (required)", cursor, HELP_BUFFER_SZ);
            }
            cursor += _ac_strcpy_safe(help, "\n", cursor, HELP_BUFFER_SZ);
        }
    }

    return help;
}

/// @brief Generate a help text string for the given @p command multi-command specification
/// @param command The multi-command to generate a help string for.
/// @result A help string if successful, otherwise @c NULL.
static char *ac_multicommand_help(struct ac_multi_command_spec const *const command) {
    char *const help = (char *) malloc(HELP_BUFFER_SZ);
    if(help == NULL) {
        return NULL;
    }

    size_t cursor = 0;

    if(command->help != NULL) {
        cursor += _ac_strcpy_safe(help, command->help, cursor, HELP_BUFFER_SZ);
        cursor += _ac_strcpy_safe(help, "\n", cursor, HELP_BUFFER_SZ);
    }

    size_t max_command_name_len = 0;
    for(size_t i = 0; i < command->n_subcommands; i++) {
        assert(command->subcommands[i].name != NULL);
        size_t const name_len = strnlen(command->subcommands[i].name, MAX_STRING_LEN);
        max_command_name_len  = name_len > max_command_name_len ? name_len : max_command_name_len;
    }

    cursor += _ac_strcpy_safe(help, "\nCommands:\n", cursor, HELP_BUFFER_SZ);
    for(size_t i = 0; i < command->n_subcommands; i++) {
        cursor += _ac_strcpy_safe(help, "  ", cursor, HELP_BUFFER_SZ);
        cursor += _ac_strcpy_safe(help, command->subcommands[i].name, cursor, HELP_BUFFER_SZ);

        size_t const name_len = strnlen(command->subcommands[i].name, MAX_STRING_LEN);
        for(size_t j = 0; j < (max_command_name_len - name_len) + 1; j++) {
            cursor += _ac_strcpy_safe(help, " ", cursor, HELP_BUFFER_SZ);
        }

        switch(command->subcommands[i].type) {
            case COMMAND_SINGLE: {
                if(command->subcommands[i].single.command->help) {
                    cursor += _ac_strcpy_safe(help, command->subcommands[i].single.command->help,
                                              cursor, HELP_BUFFER_SZ);
                }
                break;
            }
            case COMMAND_MULTI: {
                if(command->subcommands[i].multi.subcommands->help) {
                    cursor += _ac_strcpy_safe(help, command->subcommands[i].multi.subcommands->help,
                                              cursor, HELP_BUFFER_SZ);
                }
                break;
            }
        }
        cursor += _ac_strcpy_safe(help, "\n", cursor, HELP_BUFFER_SZ);
    }

    return help;
}

/// @brief Determine if the provided `command` is a valid spec, therefore may safely be passed to
/// `ac_parse_command`.
/// @result `AC_ERROR_SUCCESS` when the `command` is valid.
static struct ac_status ac_validate_command(struct ac_command_spec const *const command) {
    if(command == NULL) {
        return (struct ac_status)(struct ac_status) {.code = AC_ERROR_INVALID_PARAMETER};
    }

    for(size_t i = 0; i < command->n_arguments; i++) {
        struct ac_argument_spec const *const arg = &command->arguments[i];
        if(arg->name == NULL) {
            return (struct ac_status) {.code    = AC_ERROR_ARGUMENT_SPEC_NEEDS_NAME,
                                       .context = (void *) i};
        }
    }

    for(size_t i = 0; i < command->n_options; i++) {
        struct ac_option_spec const *const option = &command->options[i];
        if(option->long_name == NULL) {
            return (struct ac_status) {.code    = AC_ERROR_OPTION_SPEC_NEEDS_NAME,
                                       .context = (void *) i};
        }
        if(!_ac_string_is_alpha(option->long_name, strnlen(option->long_name, MAX_STRING_LEN))) {
            return (struct ac_status) {.code    = AC_ERROR_OPTION_LONG_NAME_INVALID,
                                       .context = (void *) i};
        }
        if(option->has_short_name && !_ac_char_is_alpha(option->short_name)) {
            return (struct ac_status) {.code    = AC_ERROR_OPTION_SHORT_NAME_INVALID,
                                       .context = (void *) i};
        }
        if(option->is_flag && option->required) {
            return (struct ac_status) {.code    = AC_ERROR_OPTION_FLAG_AND_REQUIRED,
                                       .context = (void *) i};
        }
    }

    return (struct ac_status) {.code = AC_ERROR_SUCCESS};
}

/// @brief Determine if the provided `command` is a valid multi-command spec, therefore may safely
/// be passed to `ac_parse_multicommand`.
/// @result `AC_ERROR_SUCCESS` when the `command` is valid.
static struct ac_status
ac_validate_multicommand(struct ac_multi_command_spec const *const command) {
    if(command == NULL) {
        return (struct ac_status) {.code = AC_ERROR_INVALID_PARAMETER};
    }

    for(size_t i = 0; i < command->n_subcommands; i++) {
        if(command->subcommands[i].name == NULL) {
            return (struct ac_status) {.code    = AC_ERROR_MULTICOMMAND_NEEDS_NAME,
                                       .context = (void *) i};
        }

        switch(command->subcommands[i].type) {
            case COMMAND_SINGLE: {
                struct ac_status const result =
                    ac_validate_command(command->subcommands->single.command);
                if(!ac_status_is_success(result)) {
                    return result;
                }
                break;
            }
            case COMMAND_MULTI: {
                struct ac_status const result =
                    ac_validate_multicommand(command->subcommands->multi.subcommands);
                if(!ac_status_is_success(result)) {
                    return result;
                }
                break;
            }
        }
    }

    return (struct ac_status) {.code = AC_ERROR_SUCCESS};
}

/// @brief Extracts an argument from the parsed `command` structure.
/// @result The argument value, or `NULL` if the `name` wasn't in the `command`'s arguments.
static struct ac_argument *ac_extract_argument(struct ac_command const *const command,
                                               char const *const              name) {
    for(size_t i = 0; i < command->n_arguments; i++) {
        if(0 == strncmp(command->arguments[i].argument->name, name, MAX_STRING_LEN)) {
            return &command->arguments[i];
        }
    }

    return NULL;
}

/// @brief Extracts an option from the parsed `command` structure.
/// @result The option value, or `NULL` if the `name` wasn't in the `command`'s options.
static struct ac_option *ac_extract_option(struct ac_command const *const command,
                                           char const *const              long_name) {
    for(size_t i = 0; i < command->n_options; i++) {
        if(0 == strncmp(command->options[i].option->long_name, long_name, MAX_STRING_LEN)) {
            return &command->options[i];
        }
    }

    return NULL;
}

static char *ac_error_string(struct ac_status result) {
    if(result.code == AC_ERROR_SUCCESS) {
        return NULL;
    }

    bool  include_help = false;
    char *error        = (char *) malloc(HELP_BUFFER_SZ);
#define errorf(...)                                                                                \
    snprintf(error, HELP_BUFFER_SZ, ##__VA_ARGS__);                                                \
    break

    switch(result.code) {
        case AC_ERROR_SUCCESS:
            errorf("Success\n");
        case AC_ERROR_INVALID_PARAMETER:
            errorf("Programmer error: Invalid parameter\n");
        case AC_ERROR_MEMORY_ALLOC_FAILED:
            errorf("System error: Memory allocation failed\n");
        case AC_ERROR_OPTION_NAME_NOT_IN_SPEC:
            include_help = true;
            errorf("Option name '--%s' is not valid.\n", (char *) result.context);
        case AC_ERROR_OPTION_NAME_EXPECTED:
            include_help = true;
            errorf(
                "Option value '%s' was provided where an option name was expected.",
                (char *) result.context);
        case AC_ERROR_OPTION_NAME_REQUIRED_IN_SPEC:
            include_help = true;
            errorf("Option name '--%s' is required.\n", (char *) result.context);
        case AC_ERROR_OPTION_VALUE_EXPECTED:
            include_help = true;
            errorf("Expected value for option %s\n", (char *) result.context);
        case AC_ERROR_OPTION_TOO_MANY:
            include_help = true;
            errorf("Too many options provided.\n");
        case AC_ERROR_OPTION_SPEC_NEEDS_NAME:
            errorf("Programmer error: Option in spec has a NULL long name field.\n");
        case AC_ERROR_OPTION_LONG_NAME_INVALID:
            errorf("Programmer error: Option in spec has an invalid long name field.\n");
        case AC_ERROR_OPTION_SHORT_NAME_INVALID:
            errorf("Programmer error: Option in spec has an invalid short name field.\n");
        case AC_ERROR_OPTION_FLAG_AND_REQUIRED:
            errorf("Programmer error: Option in spec has both required and is_flag.\n");
        case AC_ERROR_COMMAND_NAME_NOT_IN_SPEC:
            include_help = true;
            errorf("The command '%s' is not defined.\n", (char *) result.context);
        case AC_ERROR_COMMAND_NAME_REQUIRED:
            include_help = true;
            errorf("Another command name is expected after %s.\n",
                   (char *) result.context);
        case AC_ERROR_COMMAND_NAME_INVALID:
            include_help = true;
            errorf("The command name %s is invalid.\n", (char *) result.context);
        case AC_ERROR_ARGUMENT_MAX_EXCEEDED:
            include_help = true;
            errorf("Exceeded the maximum allowed number of arguments.\n");
        case AC_ERROR_ARGUMENT_EXCEEDED_SPEC:
            include_help = true;
            errorf("Too many arguments. Got %zu which is more than expected.\n",
                   (size_t) result.context);
        case AC_ERROR_ARGUMENT_EXPECTED_IN_SPEC:
            include_help = true;
            errorf("Missing arguments.\n");
        case AC_ERROR_ARGUMENT_SPEC_NEEDS_NAME:
            errorf("Programmer error: Argument at index %zu needs a name.\n",
                   (size_t) result.context);
        case AC_ERROR_MULTICOMMAND_NEEDS_NAME:
            errorf("Programmer error: Multi-command at index %zu needs a name.\n",
                   (size_t) result.context);
    }
#undef errorf

    if(!include_help) {
        return error;
    }

    if(result.single == NULL && result.multi == NULL) {
        return NULL;
    }

    char *help =
        result.single ? ac_command_help(result.single) : ac_multicommand_help(result.multi);
    if(help == NULL) {
        return NULL;
    }

    size_t cursor = strnlen(help, HELP_BUFFER_SZ);
    cursor += _ac_strcpy_safe(help, "\n", cursor, HELP_BUFFER_SZ);
    (void)_ac_strcpy_safe(help, error, cursor, HELP_BUFFER_SZ);
    free(error);

    return help;
}