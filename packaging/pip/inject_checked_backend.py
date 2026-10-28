# Copyright (c) 2023-2026 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

"""Switch the build backend of a generated pyproject.toml to checked_backend.

Usage: python inject_checked_backend.py <generated pyproject.toml>

The original backend is persisted in real_backend.txt (alongside to this file),
and checked_backend.py forwards all build hooks to it.
"""

import re
import sys
from pathlib import Path

pyproject_path = Path(sys.argv[1])
pyproject_contents = pyproject_path.read_text()

# Expect: build-backend = "module" or "module:object"
match = re.search(r'^build-backend\s*=\s*"([^"]+)"\s*$', pyproject_contents, re.M)
if match is None:
    sys.exit("build-backend not found (or not double-quoted) in " + str(pyproject_path))
if re.search(r"^backend-path\s*=", pyproject_contents, re.M):
    sys.exit("backend-path already set in " + str(pyproject_path) + ": merge manually")

# Record the real backend, then point the build to the in-tree wrapper
(Path(__file__).with_name("real_backend.txt")).write_text(match.group(1))
pyproject_contents = pyproject_contents.replace(
    match.group(0),
    'build-backend = "checked_backend"\nbackend-path = ["packaging/pip"]',
)
pyproject_path.write_text(pyproject_contents)
