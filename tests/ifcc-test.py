#!/usr/bin/env python3

# This scripts runs GCC as well as IFCC on each test-case provided and compares the results.
#
# input: the test-cases are specified either as individual
#         command-line arguments, or as part of a directory tree
#
# output: 
#
# The script is divided in three distinct steps:
# - in the ARGPARSE step, we understand the command-line arguments
# - in the PREPARE step, we copy all our test-cases into a single directory tree
# - in the TEST step, we actually run GCC and IFCC on each test-case
#
#

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, Optional


debug = True # TODO : replace with logging

def print_green(text):
    print("\033[92m" + text + "\033[0m")



def print_red(text):
    print("\033[91m" + text + "\033[0m")


def print_error_and_exit(text):
    print_red(text)
    sys.exit(1)


def execute_command(cmd: str, verbose=0) -> (int, str):
    """Execute a shell command and return the exit status and the output."""
    if verbose:
        print(f"debug: execute_command: {cmd}")
    try:
        output = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
        ret = 0
    except subprocess.CalledProcessError as e:
        ret = e.returncode
        output = e.output
    return ret, output.decode(sys.stdout.encoding)

def command(cmd: str, logfile=Optional[str]) -> int:
    """execute `string` as a shell command, optionally logging stdout+stderr to a file. return exit status.)"""

    return_code, output = execute_command(cmd, verbose=args.verbose)

    if logfile:
        with open(logfile, 'w') as f:
            print(output + '\n' + 'exit status: ' + str(return_code), file=f)

    return return_code


def print_file(file: Path) -> None:
    """Prints the contents of a file to stdout."""
    if not file.exists():
        print_error_and_exit(f"File {file} does not exist")
    if not file.is_file():
        print_error_and_exit(f"Path {file} is not a file")
    with open(file) as f:
        print(f.read(), end='')  # ! Why end=''?


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""

    parser = argparse.ArgumentParser(
        description="Compile multiple programs with both GCC and IFCC, run them, and compare the results.",
        epilog=""
    )

    parser.add_argument('input', metavar='PATH', nargs='+',
                        help="For each path given: if it's a file, use this file, if it's a directory, use all *.c files in this subtree")
    parser.add_argument('-d', '--debug', action="count", default=0,
                        help='Increase quantity of debugging messages (only useful to debug the test script itself)')
    parser.add_argument('-v', '--verbose', action="count", default=0,
                        help='Increase verbosity level. You can use this option multiple times.')
    parser.add_argument('--ifcc', metavar='IFCC', default='../compiler/ifcc', help='Path to the IFCC compiler')
    return parser.parse_args()


def get_c_files(inputs: List[Path]) -> List[Path]:
    """Given a directory, returns a list of all .c files the directory"""

    assert isinstance(inputs, list)
    assert all(isinstance(input, Path) for input in inputs)

    input_files: List[Path] = []

    for input in inputs:
        if not input.exists():
            print_error_and_exit(f"Path {input} does not exist")
        elif input.is_file():
            if input.suffix == '.c':
                input_files.append(input)
            else:
                print_error_and_exit(f"error: incorrect filename suffix (should be '.c'): {input}")
        elif input.is_dir():
            for root, _, files in os.walk(input):
                for file in files:
                    if file.endswith('.c'):
                        input_files.append(Path(root) / file)
        else:
            print_error_and_exit(f"error: incorrect input type (should be a file or a directory): {input}")

    return input_files


def check_files_can_be_read(files: List[Path]) -> None:
    """Check that all files can be opened for reading, and raise an error if not."""
    for file in files:
        try:
            with open(file, "r") as _:
                pass  # We don't need to do anything with the file, just check that it can be opened
        except OSError as e:
            print_error_and_exit(f"error: Unable to read file {file}: {e.strerror}")
        except Exception as e:
            print_error_and_exit(f"error: Unable to read file {file}: {e}")


Job = Path


def is_in_parent(child_path: Path, parent_path: Path) -> bool:
    """
    Check if a child path is within a parent path.
    """
    try:
        # Resolve both paths to their absolute form
        child_path = child_path.resolve()
        parent_path = parent_path.resolve()

        return child_path.is_relative_to(parent_path)
    except ValueError:
        # If ValueError is raised, child_path is not within parent_path
        return False


######################################################################################
## PREPARE step: copy all test-cases under ifcc-test-output
def prepare_test_cases(input_files: List[Path], output_dir: Path) -> List[Job]:
    """
    Prepare the test cases by copying them into a single directory tree under the output directory.

    Args:
    - input_filenames: A list of input file paths.
    - output_dir: The directory where test cases will be copied.

    Returns:
    - A list of directories containing the prepared test cases.
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    jobs: List[Job] = []

    for input_file in input_files:
        if debug >= 2:  # TODO : replace with logging
            print(f"debug: PREPARING {input_file}")

        if is_in_parent(input_file, output_dir):
            print_error_and_exit(f"error: input filename is within output directory: {input_file}")

        # Create a subdirectory for each test case with pathlib

        subdir = output_dir / str(input_file).replace('/', '-')[:-len('.c')]

        subdir.mkdir(parents=True, exist_ok=True)

        shutil.copyfile(input_file, subdir / 'input.c')

        jobs.append(subdir)

    # Eliminate duplicate paths
    unique_jobs = list(set(jobs))

    # Sort and return the unique jobs
    unique_jobs.sort()
    if debug:
        jobs_str = "\n".join([str(job) for job in unique_jobs])
        print(f"debug: unique jobs: {jobs_str}")
    return unique_jobs



def run_test_case(job: Job, test_output_dir: Path, ifcc:Path, verbose: int=1) -> bool:
    os.chdir(test_output_dir)

    print(f'TEST-CASE: {job}')
    os.chdir(job)

    ## Reference compiler = GCC
    gccstatus = command("gcc -S -o asm-gcc.s input.c", "gcc-compile.txt")
    if gccstatus == 0:
        # test-case is a valid program. we should run it
        gccstatus = command("gcc -o exe-gcc asm-gcc.s", "gcc-link.txt")
    if gccstatus == 0:  # then both compile and link stage went well
        exegccstatus = command("./exe-gcc", "gcc-execute.txt")  # Why exegccstatus is not used?
        if args.verbose >= 2:
            print_file("gcc-execute.txt")

    ## IFCC compiler
    ifccstatus = command(f"{ifcc} input.c >> asm-ifcc.s", "ifcc-compile.txt")

    if gccstatus != 0 and ifccstatus != 0:
        ## ifcc correctly rejects invalid program -> test-case ok
        print_green("TEST OK")
        return True
    if gccstatus != 0 and ifccstatus == 0:
        ## ifcc wrongly accepts invalid program -> error
        print_red("TEST FAIL (your compiler accepts an invalid program)")
        return False
    if gccstatus == 0 and ifccstatus != 0:
        ## ifcc wrongly rejects valid program -> error
        print_red("TEST FAIL (your compiler rejects a valid program)")
        if args.verbose:
            print_file("ifcc-compile.txt")
        return False

    ## ifcc accepts to compile valid program -> let's link it
    ldstatus = command("gcc -o exe-ifcc asm-ifcc.s", "ifcc-link.txt")
    if ldstatus != 0:
        print_red("TEST FAIL (your compiler produces incorrect assembly)")
        if args.verbose:
            print_file("ifcc-link.txt")
        return False

    ## both compilers  did produce an  executable, so now we  run both
    ## these executables and compare the results.

    command("./exe-ifcc", "ifcc-execute.txt")
    if open("gcc-execute.txt").read() != open("ifcc-execute.txt").read():
        print("TEST FAIL (different results at execution)")
        if args.verbose:
            print("GCC:")
            print_file("gcc-execute.txt")
            print("you:")
            print_file("ifcc-execute.txt")
        return False

    ## last but not least
    print_green("TEST OK")
    return True


if __name__ == "__main__":
    args = parse_args()

    #TODO : get ifcccompiler from args

    if args.debug >= 2:
        print('debug: command-line arguments ' + str(args))

    orig_cwd = Path.cwd()

    IFCC_TEST_OUTPUT = Path('ifcc-test-output')

    if is_in_parent(orig_cwd, IFCC_TEST_OUTPUT):
        raise ValueError("error: cannot run from within the output directory")

    if IFCC_TEST_OUTPUT.exists() and  IFCC_TEST_OUTPUT.is_dir():# TODO : do not erase the entire directory, but only the test cases
        # cleanup previous output directory
        print(f"debug: removing {IFCC_TEST_OUTPUT}")
        shutil.rmtree(IFCC_TEST_OUTPUT)

    IFCC_TEST_OUTPUT.mkdir(parents=True, exist_ok=True)

    print(f"debug: args;input = {args.input}")
    input_files = get_c_files([Path(input) for input in args.input])

    ## debug: after treewalk
    if args.debug:
        print(f"debug: list of files after tree walk: {input_files}")

    ## sanity check
    if not input_files:
        print_red("error: found no test-case in: " + " ".join(args.input))
        sys.exit(1)

    ## Here we check that  we can actually read the files.  Our goal is to
    ## fail as early as possible when the CLI arguments are wrong.
    check_files_can_be_read(input_files)

    jobs = prepare_test_cases(input_files, IFCC_TEST_OUTPUT)

    print(f"{orig_cwd=}")

    ifcc_compiler_path = Path('compiler/ifcc').resolve()

    test_results = [run_test_case(job, orig_cwd, ifcc_compiler_path) for job in jobs]

    # If any test fails (False in test_results), exit with status code 1. Otherwise, exit with 0.
    if not all(test_results):
        print_red("Some tests failed.")
        sys.exit(1)
    else:
        print_green("All tests passed.")
        sys.exit(0)
