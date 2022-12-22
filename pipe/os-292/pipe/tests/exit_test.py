import argparse
import errno
import subprocess
import sys

def parse(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', required=True)
    parser.add_argument('--test', required=True, choices=['f', 'i', 'fi', 'if'])
    return parser.parse_args(args)

def exit_test(executable, test):
    if test == 'f':
        args = ['false']
        expected = 1
    elif test == 'i':
        args = ['invalid']
        expected = errno.ENOENT
    elif test == 'fi':
        args = ['false', 'invalid']
        expected = 1
    elif test == 'if':
        args = ['invalid', 'false']
        expected = errno.ENOENT
    else:
        assert False, 'invalid test'

    p = subprocess.run([executable] + args,
                       capture_output=True, text=True, timeout=0.1)
    args = ' '.join(args)
    assert p.returncode == expected, \
           f"arguments '{args}' exited with: {p.returncode}, expected: {expected}"

def main(args):
    options = parse(args)
    exit_test(options.executable, options.test)

if __name__ == '__main__':
    main(sys.argv[1:])