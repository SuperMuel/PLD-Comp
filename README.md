﻿# PLD-Comp

## Getting started

### Features Implemented

- `char` and `int` variables as well as character and integer literals
- Arithmetic (+, - , *, /, %), bitwise (&, ^, |) and comparison (==, !=, >, >=, <, <=) operations
- Unary operators (-, !, +, ~, --x and ++x)
- variable declaration and assignment in the same line
- Verification that a variable that is declared is used and that a variable used has been declared
- while and for loops
- if and else blocks (the use of brackets is obligatory, thus else if does not work)
- functions returning int or void including putchar and getchar
- block structures and variable shadowing
- intermediate representation
- register allocation

**Important** : When declaring a function, an explicit return statement return is needed. If not, the program will compile but there will be run time errors.

### Antlr installation

To compile the compiler, you'll need to install Antlr4.
The installation steps are detailed in the [subject](https://moodle.insa-lyon.fr/pluginfile.php/198776/mod_resource/content/11/PLDComp.pdf) section 4.1.

### Configuration file

To compile the compiler, you'll need a `config.mk` in the [compiler directory](./compiler).

You can find one adapted to your system in the [compiler/example_configs](./compiler/example_configs) directory.

Take one of the files and copy it to the [compiler](./compiler) directory, then rename it to `config.mk`.

The `config.mk` should not be committed to the repository, as it is specific to your system. However, you
can add your configuration to the [compiler/example_configs](./compiler/example_configs) directory, so that
others can use it as a reference.

### Compiling the compiler

Then, you can compile the compiler by running `make` in the [compiler](./compiler) directory.


# Testing

## Writing tests
Since we are using a TDD approach, we will have tests for features that are not implemented yet. 
Thus, we split our tests into two different categories :

- **not_implemented_yet** : tests that are not implemented yet.
    These tests are here to remind us what we have to do next, but they won't block the CI. (That means
that merges can be done even if these tests are not passing)
- **passing** : tests that are implemented and passing.
    These tests are here to ensure that the code is working as expected. (You can't merge your changes into main if these tests are not passing.)
    They are here to prevent regressions.

## When to use each category

When you are working on a feature, you should write your tests in `passing`, or move them from `not_implemented_yet` to `passing`.

## Running the tests

To run all the tests `python3 tests/ifcc-test.py tests/testfiles/`

To run only the tests that must pass, `python3 tests/ifcc-test.py tests/testfiles/passing`

To run a specific test, `python3 tests/ifcc-test.py tests/testfiles/X/your_test_file.c`

The default input is `tests/testfiles/passing` so you can run `python3 tests/ifcc-test.py` to run all the tests that must pass.

