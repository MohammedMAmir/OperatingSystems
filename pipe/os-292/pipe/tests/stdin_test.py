import argparse
import os
import select
import subprocess
import sys

def parse(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', required=True)
    return parser.parse_args(args)

def stdin_test(executable):
    p = subprocess.Popen([executable, 'cat', 'cat', 'cat'],
                         stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    data = b'Test'    
    p.stdin.write(data)
    p.stdin.flush()

    rlist, _, _ = select.select([p.stdout.fileno()], [], [], 0.1)
    assert len(rlist) == 1, \
           'did not receieve data from the end of the pipe'

    received_data = p.stdout.read(len(data))
    assert data == received_data, \
           f'data from the end of the pipe did not match input, got: {received_data}, expected: {data}'
    rlist, _, _ = select.select([p.stdout.fileno()], [], [], 0.1)
    assert len(rlist) == 0, \
           'there is extra data at the end of the pipe'

    p.stdin.close()
    p.wait(timeout=0.1)
    assert p.returncode == 0, 'process did not exit successfully'

def main(args):
    options = parse(args)
    stdin_test(options.executable)

if __name__ == '__main__':
    main(sys.argv[1:])