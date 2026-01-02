# Copyright (c) 2023-2026 Orange. All rights reserved.
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

# Constants to check the presence of obsolete copyright
copyright_prefix = bytes(f"(c) 2023-", encoding="ascii")
valid_copyright = bytes(f"(c) 2023-{datetime.today().year} Orange", encoding="ascii")

# List of special files
special_files = [
    "README.txt",
    "LICENSE",
    "khiops.nsi",
    "Version.h",
    "KWKhiopsVersion.h",
]


def main(args):
    """Main method"""
    # Check obsolete copyright
    valid_copyright = True
    for file_path in args.file_paths:
        if test_obsolete_copyright(file_path):
            print(f"Copyright notice must be updated in {file_path}")
            valid_copyright = False

    # Set the return code
    return_code = 0
    if not valid_copyright:
        return_code = 1

    return return_code


def test_obsolete_copyright(file_path):
    """Check if a special file contains an obsolete copyright notice"""

    basename = os.path.basename(file_path)
    if basename in special_files:
        # Read the lines from the source file
        with open(file_path, "rb") as file:
            lines = file.readlines()

        # Check copyright
        for n, line in enumerate(lines):
            line = line.rstrip()
            if copyright_prefix in line and not valid_copyright in line:
                return True
    # Obsolete copyright not found
    return False


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="python check-obsolete-copyright.py",
        formatter_class=argparse.RawTextHelpFormatter,
        description="Checks the presence of an obsolete copyright notice of the input files",
    )
    parser.add_argument(
        "file_paths",
        metavar="FILE",
        nargs="+",
        help="One or more source files",
    )
    sys.exit(main(parser.parse_args()))
