# Copyright (c) 2024 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

"""Updates the copyright notice of the input files"""

import argparse
import filecmp
import os
import tempfile
import sys
import shutil
from datetime import datetime

# pylint: disable=line-too-long
byte_linesep = bytes(os.linesep, encoding="ascii")
copyright_banner_lines = [
    bytes(
        f"Copyright (c) {datetime.today().year} Orange. All rights reserved.",
        encoding="ascii",
    ),
    b"This software is distributed under the BSD 3-Clause-clear License, the text of which is available",
    b'at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.',
]
# pylint: enable=line-too-long


def main(args):
    """Main method"""
    # Process files and keep track if there were modifications
    were_files_modified = False
    for file_path in args.file_paths:
        is_file_modified = update_copyright(file_path)
        if is_file_modified:
            were_files_modified = True
            print(f"Updated {file_path}")

    # Set the return code
    return_code = 0
    if were_files_modified:
        return_code = 1

    return return_code


def update_copyright(file_path):
    """Updates the copyright notice of a file if necessary"""

    # Obtain the comment prefix string from the file extension
    _, ext = os.path.splitext(file_path)
    if ext in (".h", ".c", ".hpp", ".cpp", ".java"):
        comment_prefix = b"// "
    elif ext == ".py":
        comment_prefix = b"# "
    else:
        raise ValueError(f"Unsupported file extension '{ext}'.")

    # Read the lines from the source file
    with open(file_path, "rb") as file:
        lines = file.readlines()

    # Write the contents to a temporary file
    is_file_modified = False
    skipped_copyright = False
    with tempfile.NamedTemporaryFile() as tmp_stream:
        # Write the current copyright, followed by an empty line
        for line in copyright_banner_lines:
            tmp_stream.write(comment_prefix)
            tmp_stream.write(line)
            tmp_stream.write(byte_linesep)
        tmp_stream.write(byte_linesep)

        # Rewrite the file as follows
        # - Skip the old copyright
        # - Write the rest of the lines, the last one without newline
        line_number = len(lines)
        for n, line in enumerate(lines):
            line = line.rstrip()
            if (
                line.startswith(comment_prefix + b"Copyright (c)")
                or line.startswith(comment_prefix + b"This software is distributed")
                or line.startswith(comment_prefix + b"at https://spdx.org")
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
                tmp_stream.write(line)
                tmp_stream.write(byte_linesep)

        # Flush the file contents
        tmp_stream.flush()

        # If the temporary file contents do not match the original, replace it
        if not filecmp.cmp(file_path, tmp_stream.name, shallow=False):
            is_file_modified = True
            shutil.copyfile(tmp_stream.name, file_path)

    return is_file_modified


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
    sys.exit(main(parser.parse_args()))
