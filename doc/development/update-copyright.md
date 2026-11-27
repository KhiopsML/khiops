# Updating Copyright Notices Each Year

At the beginning of each year, update the copyright notices in all files to use this format: `Copyright (c) 2023-2025 Orange. All rights reserved.` Replace `2025` in this example with the current year.

Update the copyright notice manually in the following files, where it is hard-coded:
- License and package documentation:
  - `LICENSE`
  - `packaging/common/khiops/README.txt`
- Windows installer:
  - `packaging/windows/nsis/khiops.nsi`
- Tool version headers that define `#define <TOOL>_COPYRIGHT_LABEL ...`:
  - Khiops: `src/Learning/KWUtils/KWKhiopsVersion.h`
  - genum: `src/Learning/genum/Version.h`
  - khisto: `src/Learning/khisto/Version.h`
  - _khiopsgetprocnumber: `src/Norm/_khiopsgetprocnumber/Version.h`
  - genere: `src/Norm/genere/Version.h`

The `scripts/update-copyright.py` script updates the notices in all other source files automatically as part of the pre-commit hook.