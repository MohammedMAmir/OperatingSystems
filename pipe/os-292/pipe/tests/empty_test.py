import argparse
import errno
import subprocess
import sys

def parse(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', required=True)
    return parser.parse_args(args)

def empty_test(executable):
    p = subprocess.run([executable],
                       capture_output=True, text=True, timeout=0.1)
    assert p.returncode == errno.EINVAL, \
           "process did not exit with EINVAL when there's no arguments"

def main(args):
    options = parse(args)
    empty_test(options.executable)

if __name__ == '__main__':
    main(sys.argv[1:])