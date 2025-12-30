#!/usr/bin/env python3
import json
import os
import sys

def main():
    if len(sys.argv) < 3:
        print("Usage: gen_compile_commands.py <output_file> <compiler> <flags> <src1> <src2> ...")
        sys.exit(1)

    output_file = sys.argv[1]
    compiler = sys.argv[2]
    flags = sys.argv[3].split()
    sources = sys.argv[4:]

    cwd = os.getcwd()
    commands = []

    new_flags = []
    for flag in flags:
        if flag.startswith("-I"):
            path = flag[2:]
            if not os.path.isabs(path):
                path = os.path.abspath(path)
            new_flags.append(f"-I{path}")
        else:
            new_flags.append(flag)

    for src in sources:
        abs_src = os.path.abspath(src)
        commands.append({
            "directory": cwd,
            "command": f"{compiler} {' '.join(new_flags)} -c {abs_src}",
            "file": abs_src
        })

    with open(output_file, 'w') as f:
        json.dump(commands, f, indent=2)

if __name__ == "__main__":
    main()
