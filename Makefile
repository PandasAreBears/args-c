ARG_C_HEADER = args-c.h
ARG_C_TEST = test.c
COMMAND_EXAMPLE = example_command.c 
MULTI_EXAMPLE = multi_command.c 
.ONESHELL:
SHELL = zsh

CC_FLAGS := -std=c11 -g -O0 -Wall -Wpedantic -Werror -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-function

.PHONY: test docs clean

test: $(ARG_C_HEADER) $(ARG_C_TEST)
	clang -o args-c-test -g -O0 $(ARG_C_TEST)

docs: 
	doxygen Doxyfile

example: $(COMMAND_EXAMPLE)
	clang -o args-c-command $(CC_FLAGS) $(COMMAND_EXAMPLE)
	clang -o args-c-multi $(CC_FLAGS) $(MULTI_EXAMPLE)

clean:
	rm -rf $(VENV) docs args-c-test
