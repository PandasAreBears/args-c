
# Args-c

A header-only command line argument parser for the C programming language with a bunch of features that are missing from `getopt(3)`.

1. Subcommand support.
2. Help message generation.
3. Command validation (optional).

This project uses is compatible with the C11 standard.

## Terminology

- User input: An array of string provided to args-c to be parsed. Typically in the form of `&argv[1]`.
- Command Spec: The specification parsed to args-c which declaratively describes how to parse user input.
- Argument: A required, implicit value on the command line. Like unix's `cat <file>`.
- Option: A named value on the command line. Like unix's `sed -i <file>`.

## API

Callers of the args-c API define an `ac_command_spec` which describes how to parse user input. This will typically be defined in `static` memory as demonstrated in the the example files.

A help message can be generated using `ac_command_help` or `ac_multi_command_help`. This returns a string owned by the caller.

```c
char *ac_command_help(struct ac_command_spec const *const command);
char *ac_multi_command_help(struct ac_multi_command_spec const *const command);
```

To validate the command spec, caller are encouraged to use `ac_command_validate` or `ac_multi_command_validate`. This may be more appropriate in debug builds to verify that the `static` structure is valid.

```c
struct ac_status ac_command_validate(struct ac_command_spec const *const command);
struct ac_status ac_multi_command_validate(struct ac_multi_command_spec const *const command);
```

To parse user input using a command spec, use either the `ac_command_parse` or `ac_multi_command_parse`. Note, if the user input comes from `int main(int argc, char *argv[])`, then the caller likely wants to pass `argc - 1` and `&argv[1]` to these functions.

```c
struct ac_status ac_command_parse(int const argc, char const *const *const argv,
                                  struct ac_command_spec const *const command,
                                  struct ac_command *const            args);
struct ac_status ac_multi_command_parse(int const argc, char const *const *const argv,
                                        struct ac_multi_command_spec const *const root,
                                        struct ac_command *const                  args);
```

User's may provide input that is incorrect for the given command spec. The function `ac_status_is_success` is provided as a convenience for determining the success of a parsing operation.

```c
bool ac_status_is_success(struct ac_status const status);
```

If the operation was not successful, then the caller can generate a useful error string with the `ac_status_string`. The returned string is owned by the caller. Note, if the error was a user error, then the help text from `ac_command_help` is automatically included in this string.

```c
char *ac_status_string(struct ac_status result);
```

If the parsing operation was successful, then the convenience functions `ac_extract_argument` and `ac_extact_option` should be used to access the parsing result `struct ac_command *const args` values.

```c
struct ac_argument *ac_extract_argument(struct ac_command const *const command, char const *const name);
struct ac_option *ac_extract_option(struct ac_command const *const command, char const *const long_name);
```

Finally, once the caller is done with the result structure, it's underlying resources may be released with `ac_command_release`.

```c
void ac_command_release(struct ac_command *command);
```

## Example usage

Single command:
- `example_command.c`

Multi command:
- `multi_command.c`
