ARG_C_HEADER = args-c.h
ARG_C_TEST = test.c
COMMAND_EXAMPLE = example_command.c 
MULTI_EXAMPLE = multi_command.c 
.ONESHELL:
SHELL = zsh

.PHONY: test docs clean

test: $(ARG_C_HEADER) $(ARG_C_TEST)
	clang -o args-c-test -g -O0 $(ARG_C_TEST)

docs: 
	doxygen Doxyfile

example: $(COMMAND_EXAMPLE)
	clang -o args-c-command -g -O0 $(COMMAND_EXAMPLE)
	clang -o args-c-multi -g -O0 $(MULTI_EXAMPLE)

clean:
	rm -rf $(VENV) docs args-c-test
