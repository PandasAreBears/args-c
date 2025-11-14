ARG_C_HEADER = args-c.h
ARG_C_TEST = test.c
VENV = .venv
PYTHON       = $(VENV)/bin/python3
PIP          = $(VENV)/bin/pip
ACTIVATE     = . $(VENV)/bin/activate
.ONESHELL:
SHELL = zsh

.PHONY: test docs clean

test: $(ARG_C_HEADER) $(ARG_C_TEST)
	clang -o args-c-test -g -O0 test.c

docs: 
	doxygen Doxyfile


clean:
	rm -rf $(VENV) docs args-c-test
