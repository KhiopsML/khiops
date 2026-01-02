# Copyright (c) 2023-2026 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

import os
import string
import sys

# Files that must have non ASCII characters
ignored_file_paths = ["src/Learning/KWTest/KWTextParser.cpp"]

# Check the input files
file_paths = sys.argv[1:]
errors = False
for file_path in file_paths:
    # Ignore C++ files
    name, extension = os.path.splitext(file_path)
    if extension not in (".cpp", ".h"):
        sys.exit(0)

    # Ignore exceptions
    if file_path in ignored_file_paths:
        sys.exit(0)

    # Try to read the file as ASCII
    try:
        with open(file_path, encoding="ascii") as file:
            file.read()
    # On failure show the lines that contain characters out of range
    except UnicodeDecodeError:
        with open(file_path, encoding="utf-8", errors="replace") as file:
            lines = file.readlines()
            for line_number, line in enumerate(lines, 1):
                if any(ord(char) > 127 for char in line):
                    # Encoding the line to avoid another exception in the print
                    cleaned_line = line.encode("utf-8", errors="replace")
                    print(f"{file_path}:{line_number}: {cleaned_line}", end="")
        sys.exit(1)
