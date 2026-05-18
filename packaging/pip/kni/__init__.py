# Copyright (c) 2023-2026 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

"""
Khiops Native Interface (KNI) Python Package

This package provides Python bindings for the Khiops Native Interface (KNI),
enabling direct deployment of Khiops models from Python without temporary files.
"""

from .kni import KNI, KNIError

__all__ = ["KNI", "KNIError"]

# Get version from package metadata (set by scikit-build-core during wheel building)
try:
    from importlib.metadata import version

    __version__ = version("khiops-kni")
except Exception:
    # Fallback for development installations
    __version__ = "0.0.0.dev0"
