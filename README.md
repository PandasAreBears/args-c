
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
static struct ac_command_spec const command2 = {
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


int main(int argc, char *argv[]) {
    // The @c ac_command_help function is used for generating a help string. This returns an owned string,
    // so the caller is responsible outputting and freeing the buffer.
    printf("%s\n", ac_command_help(&command2));
}

```