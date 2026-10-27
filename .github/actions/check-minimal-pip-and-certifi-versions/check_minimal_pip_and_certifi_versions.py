# Copyright (c) 2023-2026 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

import sys

from importlib.metadata import PackageNotFoundError, version

from pip._vendor.packaging.version import Version

minimal_pip_version, minimal_certifi_version = sys.argv[1:3]

try:
    certifi_version = version("certifi")
except PackageNotFoundError:
    from pip._vendor import certifi

    certifi_version = certifi.__version__

version_check_failure = False
for package_name, current_version, minimal_version in [
    ("pip", version("pip"), minimal_pip_version),
    ("certifi", certifi_version, minimal_certifi_version),
]:
    if Version(current_version) < Version(minimal_version):
        print(
            f"::error::'{package_name}' {current_version} found, >= "
            f"{minimal_version} required"
        )
        version_check_failure = True
    else:
        print(
            f"'{package_name}' version is OK: {current_version} (>= "
            f"{minimal_version})"
        )
sys.exit(1 if version_check_failure else 0)
