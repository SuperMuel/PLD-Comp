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
import glob
import os
import shutil
import sys
import subprocess
from typing import List


def command(string, logfile=None):
    """execute `string` as a shell command, optionnaly logging stdout+stderr to a file. return exit status.)"""
    if args.verbose:
        print("ifcc-test.py: " + string)
    try:
        output = subprocess.check_output(string, stderr=subprocess.STDOUT, shell=True)
        ret = 0
    except subprocess.CalledProcessError as e:
        ret = e.returncode
        output = e.output

    if logfile:
        with open(logfile, 'w') as f:
            print(output.decode(sys.stdout.encoding) + '\n' + 'exit status: ' + str(ret), file=f)

    return ret


def dumpfile(name):
    """print the content of a file to stdout"""
    with open(name) as f:
        print(f.read(), end='')


def parse_args() -> argparse.Namespace:
    argparser = argparse.ArgumentParser(
        description="Compile multiple programs with both GCC and IFCC, run them, and compare the results.",
        epilog=""
    )

    argparser.add_argument('input', metavar='PATH', nargs='+', help='For each path given:'
                                                                    + ' if it\'s a file, use this file;'
                                                                    + ' if it\'s a directory, use all *.c files in this subtree')

    argparser.add_argument('-d', '--debug', action="count", default=0,
                           help='Increase quantity of debugging messages (only useful to debug the test script itself)')
    argparser.add_argument('-v', '--verbose', action="count", default=0,
                           help='Increase verbosity level. You can use this option multiple times.')
    argparser.add_argument('-w', '--wrapper', metavar='PATH',
                           help='Invoke your compiler through the shell script at PATH. (default: `ifcc-wrapper.sh`)')
    return argparser.parse_args()


def get_c_files(path: str) -> List[str]:
    """return a list of all .c files in a directory tree"""
    inputfilenames = []
    for path in args.input:
        path = os.path.normpath(path)  # collapse redundant slashes etc.
        if os.path.isfile(path):
            if path[-2:] == '.c':
                inputfilenames.append(path)
            else:
                print("error: incorrect filename suffix (should be '.c'): " + path)
                exit(1)
        elif os.path.isdir(path):
            for dirpath, dirnames, filenames in os.walk(path):
                inputfilenames += [dirpath + '/' + name for name in filenames if name[-2:] == '.c']
        else:
            print("error: cannot read input path `" + path + "'")
            sys.exit(1)

    return inputfilenames


def check_files_can_be_read(files: List[str]) -> None:
    """check that all files can be opened for reading, and exit if not."""
    for file in files:
        try:
            with open(file, "r") as f:
                pass  # We don't need to do anything with the file, just check that it can be opened
        except OSError as e:
            print(f"error: Unable to read file {file}: {e.strerror}")
            sys.exit(1)
        except Exception as e:
            print(f"error: Unable to read file {file}: {e}")
            sys.exit(1)


def get_wrapper_path(args: argparse.Namespace) -> str:
    """return the path to the wrapper script"""
    if args.wrapper:
        wrapper = os.path.realpath(os.getcwd() + "/" + args.wrapper)
    else:
        wrapper = os.path.dirname(
            os.path.realpath(__file__)) + "/ifcc-wrapper.sh"  # TODO: set this directily in the argparse default

    if not os.path.isfile(wrapper):
        print("error: cannot find " + os.path.basename(wrapper) + " in directory: " + os.path.dirname(wrapper))
        exit(1)

    return wrapper


def check_wrapper_can_be_executed(wrapper: str) -> None:
    """check that the wrapper script can be executed"""
    if not os.access(wrapper, os.X_OK):
        print(f"error: {wrapper} is not executable")
        sys.exit(1)


######################################################################################
## PREPARE step: copy all test-cases under ifcc-test-output
def prepare_test_cases(input_filenames: list, output_dir: str, debug: bool) -> list:
    """
    Prepare the test cases by copying them into a single directory tree under the output directory.

    Args:
    - input_filenames: A list of input file paths.
    - output_dir: The directory where test cases will be copied.
    - debug: Flag to indicate if debug messages should be printed.

    Returns:
    - A list of directories containing the prepared test cases.
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    jobs = []

    for inputfilename in input_filenames:
        if debug >= 2:
            print(f"debug: PREPARING {inputfilename}")

        if output_dir in os.path.realpath(inputfilename):
            print(f'error: input filename is within output directory: {inputfilename}')
            sys.exit(1)

        # Create a subdirectory for each test case
        subdir = os.path.join(output_dir, os.path.basename(inputfilename)[:-2].replace('/', '-'))
        os.makedirs(subdir, exist_ok=True)
        shutil.copyfile(inputfilename, os.path.join(subdir, 'input.c'))
        jobs.append(subdir)

    # Eliminate duplicate paths
    unique_jobs = list(set(jobs))

    # Sort and return the unique jobs
    unique_jobs.sort()
    if debug:
        print("debug: list of test-cases after deduplication:", " ".join(unique_jobs))

    return unique_jobs


def run_test_case(jobname: str, orig_cwd: str, wrapper: str, verbose: int) -> None:
    os.chdir(orig_cwd)

    print('TEST-CASE: ' + jobname)
    os.chdir(jobname)

    ## Reference compiler = GCC
    gccstatus = command("gcc -S -o asm-gcc.s input.c", "gcc-compile.txt")
    if gccstatus == 0:
        # test-case is a valid program. we should run it
        gccstatus = command("gcc -o exe-gcc asm-gcc.s", "gcc-link.txt")
    if gccstatus == 0:  # then both compile and link stage went well
        exegccstatus = command("./exe-gcc", "gcc-execute.txt")  # Why exegccstatus is not used?
        if args.verbose >= 2:
            dumpfile("gcc-execute.txt")

    ## IFCC compiler
    ifccstatus = command(wrapper + " asm-ifcc.s input.c", "ifcc-compile.txt")

    if gccstatus != 0 and ifccstatus != 0:
        ## ifcc correctly rejects invalid program -> test-case ok
        print("TEST OK")
        return
    if gccstatus != 0 and ifccstatus == 0:
        ## ifcc wrongly accepts invalid program -> error
        print("TEST FAIL (your compiler accepts an invalid program)")
        return
    if gccstatus == 0 and ifccstatus != 0:
        ## ifcc wrongly rejects valid program -> error
        print("TEST FAIL (your compiler rejects a valid program)")
        if args.verbose:
            dumpfile("ifcc-compile.txt")
        return

    ## ifcc accepts to compile valid program -> let's link it
    ldstatus = command("gcc -o exe-ifcc asm-ifcc.s", "ifcc-link.txt")
    if ldstatus != 0:
        print("TEST FAIL (your compiler produces incorrect assembly)")
        if args.verbose:
            dumpfile("ifcc-link.txt")
        return

    ## both compilers  did produce an  executable, so now we  run both
    ## these executables and compare the results.

    command("./exe-ifcc", "ifcc-execute.txt")
    if open("gcc-execute.txt").read() != open("ifcc-execute.txt").read():
        print("TEST FAIL (different results at execution)")
        if args.verbose:
            print("GCC:")
            dumpfile("gcc-execute.txt")
            print("you:")
            dumpfile("ifcc-execute.txt")
        return

    ## last but not least
    print("TEST OK")


if __name__ == "__main__":
    args = parse_args()

    if args.debug >= 2:
        print('debug: command-line arguments ' + str(args))

    orig_cwd = os.getcwd()

    IFCC_TEST_OUTPUT = 'ifcc-test-output'

    if IFCC_TEST_OUTPUT in orig_cwd:
        print('error: cannot run from within the output directory')
        exit(1)

    if os.path.isdir(IFCC_TEST_OUTPUT):
        # cleanup previous output directory
        command(f'rm -rf {IFCC_TEST_OUTPUT}')

    os.mkdir(IFCC_TEST_OUTPUT)

    inputfilenames = get_c_files(args.input)

    ## debug: after treewalk
    if args.debug:
        print("debug: list of files after tree walk:", " ".join(inputfilenames))

    ## sanity check
    if not inputfilenames:
        print("error: found no test-case in: " + " ".join(args.input))
        sys.exit(1)

    ## Here we check that  we can actually read the files.  Our goal is to
    ## fail as early as possible when the CLI arguments are wrong.
    check_files_can_be_read(inputfilenames)

    ## Last but not least: we now locate the "wrapper script" that we will
    ## use to invoke ifcc
    wrapper = get_wrapper_path(args)

    ## and we check that it is executable
    check_wrapper_can_be_executed(wrapper)

    if args.debug:
        print("debug: wrapper path: " + wrapper)

    jobs = prepare_test_cases(inputfilenames, IFCC_TEST_OUTPUT, args.debug)

    for jobname in jobs:
        run_test_case(jobname, orig_cwd, wrapper, args.verbose)
