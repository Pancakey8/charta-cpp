#!/usr/bin/env python3
import re
import subprocess
import sys
from pathlib import Path

def main():
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <input> <output>", file=sys.stderr)
        sys.exit(1)

    in_path = Path(sys.argv[1])
    out_path = Path(sys.argv[2])

    text = in_path.read_text()

    # Match: _mangle_(some_stuff, "foobar")
    pattern = re.compile(r'_mangle_\([^,]*,\s*"([^"]*)"\)')

    cache = {}

    def replace(match):
        ascii_arg = match.group(1)

        if ascii_arg not in cache:
            result = subprocess.run(
                ["./mangler", ascii_arg],
                check=True,
                stdout=subprocess.PIPE,
                text=True,
            )
            cache[ascii_arg] = result.stdout.strip()

        return cache[ascii_arg]

    new_text = pattern.sub(replace, text)
    out_path.write_text(new_text)

if __name__ == "__main__":
    main()

