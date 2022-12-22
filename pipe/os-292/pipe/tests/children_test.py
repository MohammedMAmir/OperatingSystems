import argparse
import subprocess
import sys

def parse(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', required=True)
    parser.add_argument('--children', required=True, type=int)
    return parser.parse_args(args)

def children_test(executable, children):
    assert children > 0
    args = ['true'] * children

    p = subprocess.run(['strace', '-e', 'clone,wait4', executable] + args,
                       capture_output=True, text=True, timeout=0.1)

    child_pids = set()
    wait_pids = set()
    lines = p.stderr.splitlines()
    for line in lines:
        if line.startswith('clone'):
            _, pid = line.split(' = ')
            if 'ERESTARTNOINTR' in pid:
                continue
            assert pid.isdigit(), f'clone (fork) did not return a valid pid, got: {pid}'
            pid = int(pid)
            child_pids.add(pid)
        elif line.startswith('wait4'):
            _, pid = line.split(' = ')
            if 'ERESTARTSYS' in pid:
                continue
            assert pid.isdigit(), \
                   f'wait4 (wait or waitpid) did not return a valid pid, got: {pid}'
            pid = int(pid)
            assert pid in child_pids
            wait_pids.add(pid)

    num_children = len(child_pids)
    assert num_children == children, \
           f'children created: {num_children}, expected: {children}'

    for pid in wait_pids:
        child_pids.remove(pid)

    num_orphans = len(child_pids)
    orphans = ', '.join([str(i) for i in child_pids])
    assert num_orphans == 0, \
           f'{num_orphans} orphan(s) detected: {orphans}'

    assert p.returncode == 0, 'process did not exit successfully'

def main(args):
    options = parse(args)
    children_test(options.executable, options.children)

if __name__ == '__main__':
    main(sys.argv[1:])