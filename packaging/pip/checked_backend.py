# Copyright (c) 2023-2026 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

"""In-tree PEP 517 backend wrapper.

Checks that pip (and certifi) in the isolated build environment satisfy the
minimal versions, then forwards every build hook to the real backend.
"""

import importlib
import importlib.util
import os
import subprocess
import sys
from pathlib import Path

# packaging/pip/checked_backend.py -> repository root is parents[2]
CHECK_SCRIPT = (
    Path(__file__).resolve().parents[2]
    / ".github"
    / "actions"
    / "check-minimal-pip-and-certifi-versions"
    / "check_minimal_pip_and_certifi_versions.py"
)

pip_minimal_version = os.environ.get("MINIMAL_PIP_VERSION")
certifi_minimal_version = os.environ.get("MINIMAL_CERTIFI_VERSION")

if not (pip_minimal_version and certifi_minimal_version):
    # Visible in the build log: shows the variables were not forwarded (Linux)
    print("checked_backend: MINIMAL_* variables not set: check skipped")
elif importlib.util.find_spec("pip") is None:
    # No pip in this environment means no pip below the minimum
    print("checked_backend: pip not present in the build environment: nothing to check")
elif not CHECK_SCRIPT.is_file():
    # Fail rather than skip, so a missing script is never silent
    sys.exit(f"checked_backend: {CHECK_SCRIPT} not found")
else:
    # sys.executable is the isolated environment's interpreter, so the script
    # inspects the pip (and certifi) of this environment
    result = subprocess.run(
        [
            sys.executable,
            str(CHECK_SCRIPT),
            pip_minimal_version,
            certifi_minimal_version,
        ]
    )
    if result.returncode != 0:
        sys.exit(
            "checked_backend: pip or certifi version check failed in the isolated build environment"
        )
    print(
        "checked_backend: pip and certifi versions are OK in the isolated build environment"
    )

# Load the real backend recorded by inject_checked_backend.py
_spec = Path(__file__).with_name("real_backend.txt").read_text().strip()
_module, _, _object = _spec.partition(":")
_backend = importlib.import_module(_module)
if _object:
    _backend = getattr(_backend, _object)


def __getattr__(attribute):
    # PEP 562: forward build_wheel, build_sdist and the optional hooks.
    # Hooks the real backend lacks still raise AttributeError, so the
    # frontend's optional-hook detection keeps working.
    if attribute.startswith("__"):
        raise AttributeError(attribute)
    return getattr(_backend, attribute)
