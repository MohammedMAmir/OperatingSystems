import argparse
import subprocess
import sys

def parse(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', required=True)
    parser.add_argument('--pipes', required=True, type=int)
    return parser.parse_args(args)

def pipe_test(executable, pipes):
    assert pipes > 0

    expected = subprocess.run(['uname'], capture_output=True, text=True).stdout
    args = ['uname'] + ['cat'] * (pipes)
    p = subprocess.run([executable] + args,
                       capture_output=True, text=True, timeout=0.1)
    args = ' '.join(args)
    assert p.stdout == expected, \
           f"arguments '{args}' got: {repr(p.stdout)}, expected: {repr(expected)}"

    args = ['uname'] + ['cat'] * (pipes - 1) + ['true']
    p = subprocess.run([executable] + args,
                       capture_output=True, text=True, timeout=0.1)
    args = ' '.join(args)
    assert p.stdout == '', \
           f"arguments '{args}' got: {repr(p.stdout)}, expected no output"

    assert p.returncode in [0, 1, 13, 141], 'process did not exit successfully'

def main(args):
    options = parse(args)
    pipe_test(options.executable, options.pipes)

if __name__ == '__main__':
    main(sys.argv[1:])
