# Khiops NSIS packaging
This folder contains the scripts to generate the Khiops Windows installer. It is built with
[NSIS](https://nsis.sourceforge.io/Download). See also the [Release Process wiki
page](https://github.com/KhiopsML/khiops/wiki/Release-Process).

## What the installer does
Besides installing the Khiops executables, the installer automatically detects the presence of:
- [Microsoft MPI](https://learn.microsoft.com/en-us/message-passing-interface/microsoft-mpi)

and installs it if necessary.


It also installs:
- The [Khiops Visualization](https://github.com/khiopsrelease/kv-release/releases/latest) and
  [Khiops Covisualization](https://github.com/khiopsrelease/kc-release/releases/latest) apps by
  executing their corresponding installers.
- The JRE from [Eclipse Temurin](https://adoptium.net/fr/temurin/releases/)
- The [sample datasets](https://github.com/KhiopsML/khiops-samples/releases/latest).
- Documentation files:
  - PDF Guides .
  - README.txt and WHATSNEW.txt (obtained from the sources at (../../common/khiops))

## How to obtain the package assets
All the package assets (installers, documentation, etc) are available at the
[`khiops-win-install-assets`](https://github.com/KhiopsML/khiops-win-install-assets/releases/latest)
repository.

## How to build the installer manually
1) Install NSIS and make sure `makensis` it is available in the `%PATH%`.
2) Download and decompress the package assets to your machine.
3) [Build Khiops in Release mode](https://github.com/KhiopsML/khiops/wiki/Building-Khiops)
4) In a console, go to the `packaging/windows/nsis` directory and execute
```bat
%REM We assume the package assets were downoladed to packaging\windows\nsis\assets
makensis ^
   /DKHIOPS_VERSION=10.2.0-preview ^
   /DKHIOPS_REDUCED_VERSION=10.2.0 ^
   /DKHIOPS_WINDOWS_BUILD_DIR=..\..\..\build\windows-msvc-release ^
   /DJRE_PATH=.\assets\jre\ ^
   /DMSMPI_INSTALLER_PATH=.\assets\msmpisetup.exe ^
   /DMSMPI_VERSION=10.1.3 ^
   /DKHIOPS_VIZ_INSTALLER_PATH=.\assets\khiops-visualization-Setup-11.0.2.exe ^
   /DKHIOPS_COVIZ_INSTALLER_PATH=.\assets\khiops-covisualization-Setup-10.2.4.exe ^
   /DKHIOPS_SAMPLES_DIR=.\assets\samples ^
   /DKHIOPS_DOC_DIR=.\assets\doc ^
   khiops.nsi
```

The resulting installer will be at `packaging/windows/nsis/khiops-10.1.1-setup.exe`.

_Note 1_: See [below](#build-script-arguments) for the details of the installer builder script arguments.

_Note 2_: If your are using powershell replace the `^` characters by backticks `` ` `` in the
multi-line command.


## Github Workflow
This process is automatized in the [pack-nsis.yml workflow](../../../.github/workflows/pack-nsis.yml).

## Build script arguments
All the arguments are mandatory except for `DEBUG`, they must be prefixed by `/D` and post fixed by
`=<value>` to specify a value.

- `DEBUG`: Enables debug messages in the installer. They are "OK" message boxes.
- `KHIOPS_VERSION`: Khiops version for the installer.
- `KHIOPS_REDUCED_VERSION`: Khiops version without suffix and only digits and periods.
- `KHIOPS_WINDOWS_BUILD_DIR`: Build directory for (usually `build\windows-msvc-release` relative to
  the project root).
- `JRE_PATH`: Path to the Java Runtime Environment (JRE) directory.
- `MSMPI_INSTALLER_PATH`: Path to the Microsoft MPI (MS-MPI) installer.
- `MSMPI_MPI_VERSION`: MS-MPI version.
- `KHIOPS_VIZ_INSTALLER_PATH`: Path to the Khiops Visualization installer.
- `KHIOPS_COVIZ_INSTALLER_PATH`: Path to the Khiops Covisualization installer.
- `KHIOPS_SAMPLES_DIR`: Path to the sample datasets directory.
- `KHIOPS_DOC_DIR`: Path to the directory containing the documentation.
