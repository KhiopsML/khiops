"""Updates the copyright notice of the input files"""

import argparse
import os
from datetime import datetime

# pylint: disable=line-too-long
byte_linesep = bytes(os.linesep, encoding="ascii")
copyright_banner_lines = [
    bytes(
        f"// Copyright (c) {datetime.today().year} Orange. All rights reserved.",
        encoding="ascii",
    ),
    b"// This software is distributed under the BSD 3-Clause-clear License, the text of which is available",
    b'// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.',
]
# pylint: enable=line-too-long


def main(args):
    """Main method"""
    for file_path in args.file_paths:
        update_copyright(file_path)


def update_copyright(file_path):
    """Updates the copyright notice of a file"""
    print(f"Updating {file_path}")

    # Read the lines from the source file
    with open(file_path, "rb") as file:
        lines = file.readlines()

    # Then write the file as-is
    skipped_copyright = False
    with open(file_path, "wb") as file:
        # Write the current copyright, followed by an empty line
        for line in copyright_banner_lines:
            file.write(line)
            file.write(byte_linesep)
        file.write(byte_linesep)

        # Rewrite the file as follows
        # - Skip the old copyright
        # - Write the rest of the lines, the last one without newline
        line_number = len(lines)
        for n, line in enumerate(lines):
            line = line.rstrip()
            if (
                line.startswith(b"// Copyright (c)")
                or line.startswith(b"// This software is distributed")
                or line.startswith(b"// at https://spdx.org")
            ) and not skipped_copyright:
                continue
            else:
                if not skipped_copyright:
                    # Skip head empty lines
                    if len(line) == 0:
                        continue
                    else:
                        skipped_copyright = True
                # Beware: all lines must end with an end of line, including the last line
                # (otherwise, the Windows RC compiler does not work)
                file.write(line)
                file.write(byte_linesep)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="python update_copyright.py",
        formatter_class=argparse.RawTextHelpFormatter,
        description="Updates the copyright notice of the input files",
    )
    parser.add_argument(
        "file_paths",
        metavar="FILE",
        nargs="+",
        help="One or more source code files",
    )
    main(parser.parse_args())
