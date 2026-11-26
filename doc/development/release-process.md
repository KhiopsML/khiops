# Release Process

## Release Checklist

_Note: At the [bottom of this page](#release-checklist-code), you'll find the Markdown source for this checklist, which you can paste into a release issue._

- **Preparation**
  - [ ] Create a `prepare-release-x.y.z` branch based on the `main` (or `main-v10`) branch, then check it out
  - [ ] Update the `README.txt` and `WHATSNEW.txt` in the `packaging/common/khiops` directory
  - [ ] Update the [PDF documentation][pdf-docs]
  - [ ] Update the technical repository [khiops-win-install-assets][win-install-assets]
    - Update the versions of all assets that require an update, namely:
      - Khiops PDF Docs
      - Java JRE installer
      - MS MPI installer
  - [ ] Update the Windows assets tag/version in the Windows installer workflow (`pack-nsis.yml`)
  - [ ] Update the Khiops version to `x.y.z-(a|b|rc).w` in the `src/Learning/KWUtils/KWKhiopsVersion.h` header file
  - [ ] Create a PR from the `prepare-release-x.y.z` branch and have it reviewed
  - [ ] Once the PR is approved, merge it into the `main` branch
  - [ ] Create the `x.y.z-(a|b|rc).w` pre-release tag on the `main` branch (see [Versioning](#versioning))
    - Example: `git tag 10.2.1-a.1 main`
- **Full Test Loop**
  - [ ] Push the `x.y.z-(a|b|rc).w` pre-release tag
    - Example: `git push origin tag 10.2.1-a.1`
    - This will create a pre-release entry in the [Releases section][khiops-releases] containing all CI/CD-generated packages.
  - [ ] Publish the Conda packages `x.y.z` to the `conda-forge` Conda channel. [See the dedicated Conda section](#publish-the-conda-packages-xyz-to-the-conda-forge-conda-channel)
  - [ ] Publish the pip packages to TestPyPI:
    - Run the manual "pip packaging" action. Select the current tag as the `branch` input and `testpypi` as the Python repository.
  - [ ] Test the conda packages with the manual action `Test Conda Packages`.
  - [ ] Run the end-to-end tests (also known as `LearningTest`) in all supported environments
    - If you find major issues, create issues and PRs to address them, then return to "Create pre-release tag..."
  - [ ] Execute the [manual tests for the GUI packages](#manual-testing-procedure)

- **Finalize the Release**
  - [ ] Update `src/Learning/KWUtils/KWKhiopsVersion.h` to the final release version, `x.y.z`
  - [ ] Create the final release tag (see also [Versioning](#versioning)):
      - Tag the `main` branch (`git tag x.y.z main` then `git push origin tag x.y.z`)
      - This will create an entry in the [Releases section][khiops-releases] containing all CI/CD-generated packages.
  - [ ] Sign the Windows installer:
    - Run the [Windows installer workflow][win-installer-wf] from the tag and select the "Sign installer and binaries" checkbox.
    - This will replace the Windows installer in the release with a signed version.
  - [ ] Copy the release notes from [packaging/common/khiops/WHATSNEW.txt](../../packaging/common/khiops/WHATSNEW.txt) to the new [GitHub release entry][khiops-releases].
  - [ ] Remove any packages that are not intended for release.
    - We create additional packages for testing purposes (for example, `kni-transfer`).
  - [ ] Publish the Conda packages `x.y.z` to the `conda-forge` Conda channel. [See the dedicated Conda section](#publish-the-conda-packages-xyz-to-the-conda-forge-conda-channel)
  - [ ] Publish the pip packages to PyPI with **signed** binaries:
    - Run the manual "pip packaging" action. Select the current tag as the `branch` input, `pypi` as the Python repository, and check the "Sign Windows binaries ..." box.
  - [ ] Publish the pip packages to TestPyPI to facilitate downstream CI/CD processes for enabler packaging and delivery:
    - Run the manual "pip packaging" action. Select the current tag as the `branch` input and `testpypi` as the Python repository.
  - [ ] Set the release as "latest" and unset its "pre-release" status.
  - [ ] Update the [KNI tutorial](https://github.com/KhiopsML/KNI-tutorial) repository by running the `update-kni-tutorial` workflow from the tag you just created.
  - [ ] Release the Linux packages on DCR with help from the GitLab repository `release_khiops_packages`.
  - [ ] Release the `khiops-python` enabler.
  - [ ] Release the `khiops-docker` enabler.
  - [ ] Release the `khiops-notebook` enabler.
    - **At this point the update of the website is unblocked**

### Publish the Conda packages `x.y.z` to the `conda-forge` Conda channel
  - [ ] Create a _personal_ fork of the GitHub repository `https://github.com/conda-forge/khiops-binaries-feedstock`.
  - [ ] Create a branch named `update-version-to-x-y-z` in your `khiops-binaries-feedstock` fork.
  - [ ] Update the following fields in `khiops-binaries-feedstock/recipe/meta.yaml` on the new branch:
    - Set `khiops_version` to `x.y.z`.
    - Set `source: sha256` to the SHA-256 checksum of the source tarball for the `x.y.z` GitHub release:
         ```bash
         curl -sL  https://github.com/KhiopsML/khiops/archive/refs/tags/x.y.z.tar.gz | openssl sha256 | awk '{print $NF}'
         ```
    - [ ] In `recipes/conda_build_config.yaml`, set the `channel_targets: - conda-forge <label>` value to `main` for official releases or `rc` for pre-releases.
    - [ ] Create a pull request from the new branch in the `khiops-binaries-feedstock` repository.
    - [ ] Request that the repository be re-rendered for the PR, following the instructions in the PR.

[khiops-releases]: https://github.com/KhiopsML/khiops/releases
[manual-tests]: https://github.com/KhiopsML/khiops/wiki/Release-Process#manual-testing-procedure
[pdf-docs]: https://github.com/khiopsml/khiops-pdf-docs
[win-install-assets]: https://github.com/khiopsml/khiops-win-install-assets
[win-installer-wf]: https://github.com/KhiopsML/khiops/actions/workflows/pack-nsis.yml

## Versioning

### Khiops Core
The Khiops versioning scheme uses the format `MAJOR.MINOR.PATCH{-PRE_RELEASE.PR_INCREMENT}`, where:
- `MAJOR` is a number that identifies a major release. A major release includes major and minor features, possible API changes, and bug fixes.
- `MINOR` is a number that identifies a minor release, which includes minor features and bug fixes.
- `PATCH` is a number that identifies a patch release, which contains only bug fixes.
- `PRE_RELEASE` is one of `a`, `b`, or `rc` and indicates a pre-release status:
  - `a` is an Alpha release: a stable version of the code that may be released for private testing.
  - `b` is a Beta release for public testing.
  - `rc` is a Release Candidate. If no major bugs are found, it may become the final release.
- `PR_INCREMENT` is the numerical increment for a pre-release.

Any `MAJOR`, `MINOR`, or `PATCH` release is widely available, so enablers such as `khiops-python` or `khiops-docker` must be upgraded as part of the release.

After the final release in a major series, all subsequent versions are alpha until the beta for the next version is declared.

For example, the last version in the 10 series is 10.3.x. Development versions are `11.0.0-b.0` until the alpha version of the next major release is available.

Unlike major releases, pre-releases are not intended to remain available permanently. The GitHub release containing a pre-release will be removed when the next (pre-)release is published, although the Git tag will remain.

Note about package versions: when a package changes but its binaries do not, a new minor version is released. For example, this applies when the Windows installer is updated to include a new version of the Khiops visualization tool.

### Khiops Enablers
Enablers should use the Khiops version as a base and add a suffix to create their own version numbers.

- `khiops-python`: https://github.com/KhiopsML/khiops-python/blob/main/CONTRIBUTING.md#versioning
- `khiops-docker`: TBD
- `khiops-visualization`: TBD (exception to the rule)

### Drivers
The driver version starts with the driver API version. API version 0 was introduced in Khiops 10.0.0 and is defined in [SystemFileDriver.h](../../src/Norm/base/SystemFileDriver.h).

### Git Tags
Release tags are created only on the `main` branch. Pre-release tags may occasionally be created on a feature branch.
Tags trigger the creation of an entry in the GitHub repository's "Releases" section. This entry contains all Linux and Windows packages. Any packages that fail the end-to-end tests should be removed from the final release.

Avoid deleting tags that are not alpha releases, as downstream packages (e.g., `khiops-python`) may depend on them.

## Manual Testing Procedure
Below are the checklists for the manual tests to perform on Khiops packages before release.
Variants for each step are listed as sub-bullets.

Report the result of each step as one of the following:
- OK
- OK with a warning:
  - The step works, but there is unexpected behavior or a popup.
    - Consider adding a warning to the installation guide or troubleshooting section.
- KO
  - Provide as much context as possible so developers can reproduce the problem.

### Windows

Perform the checklist below on the following system combinations:

Versions:
- Windows 10
- Windows 11

Machine types:
- Personal/home machine
- Orange IT machines

#### Checklist

- Check that the download link on https://khiops.org works correctly:
  - In Chrome
  - In Edge
  - In Firefox
- Run the installer:
  - By right-clicking the installer and selecting "Run as administrator"
  - By double-clicking the installer executable (this is not equivalent when using a standard user account)
    - Check that it is signed by Orange and does not display "Unknown Publisher"/"Editeur Inconnu".
- Select the installation directory
  - The default: `C:\Program Files\khiops`
  - One within the user home directory (e.g., `C:\Users\<USERNAME>\khiops`)
  - Another directory (e.g., `C:\Temp\khiops` or `C:\Applications\khiops`)
- Launch the Khiops applications:
  - From the start menu
  - From the desktop icon
  - From the shortcut of the installation directory
- Check the version and year (e.g., 2024) in the application and documentation:
  - In the application menu: `Help/About Khiops...`
  - In the text documents in the installation directory: `LICENSE.txt`, `README.txt`, and `WHATSNEW.txt`
  - In the guides and tutorial PDF files in the installation directory's `doc` subdirectory
- Check the following in the application:
  - The "Open Dictionary File..." menu opens in `C:\Users\Public\khiops_data\samples`.
  - `Adult` supervised analysis runs successfully.
  - The resulting `AllReports.khj` works correctly in Khiops Visualization.
- Uninstall the application:
  - From the start menu
  - From the "Apps & Features"/"Applications et Fonctionnalités" pane in the settings
- Check that the following files and directories were removed:
  - The installation dir
  - The `C:\Users\Public\khiops_data\samples` directory
    - Confirm that any user-generated files from the `Adult` analysis above have not been removed.
- Install or reinstall:
  - Uninstall everything if necessary.
  - Install using any method.
  - Install again:
    - In the same directory
    - In another directory
  - Check that a popup saying "Khiops is already installed" appears, and accept it.
  - When installing in another directory, check that the previous installation was removed.

### Linux

Perform the checklist below on the following operating systems:

Operating systems:
- Ubuntu 20.04
- Ubuntu 22.04
- Ubuntu 24.04
- Debian 11
- Debian 12
- Debian 13
- Rocky Linux 8
- Rocky Linux 9

#### Checklist
- Install using the appropriate method for each operating system.
- Check the output of `khiops_env`:
  ```sh
  khiops_env --env
  ```
- Use Docker images to test on several Linux distributions. Run the following commands on the _host_ system (where Docker is launched):
  ```sh
  # Enable X clients to connect from any host (not only localhost)
  xhost +

  # Display graphics windows from the Docker container on the same screen address as the host computer
  # Share UNIX sockets between the Xorgs of the host and Docker container
  docker run -it --network=host -e DISPLAY=$DISPLAY -v  /tmp/.X11-unix:/tmp/.X11-unix:ro os_codename:os_version

  # After Docker session is finished, secure access to the host's X server again
  xhost -
  ```
  Then install and launch the Khiops desktop application from the shell as usual.
- Launch the Khiops applications:
  - From the start menu
  - From the desktop icon (?)
  - From a shell in Docker containers, using the `khiops` and `khiops_coclustering` commands
- Check the following in the application:
  - The "Open Dictionary File..." menu opens in `/usr/bin` (or in the current directory from which the `khiops` or `khiops_coclustering` command was launched).
  - `Adult` supervised analysis runs successfully in `khiops`.
  - The resulting `AllReports.khj` works correctly in Khiops Visualization. If testing in Docker containers, launch Khiops Visualization on the _host_ machine.
  - `khiops -v` and `khiops -s` work when run from a shell.
  - In a Docker container without a display enabled, running `khiops` or `khiops_coclustering` without arguments, or with any argument other than `-b`, `-s`, or `-v`, displays `GUI is not available, please use the '-b' flag`. If an unknown argument is passed with `-b`, the command's help is displayed.
- Uninstall the application.
- Check that the following files and directories were removed:
  - `MODL*`, `khiops`, `khiops_env`, and `khiops-coclustering` from `/usr/bin`
  - `norm.jar` and `khiops.jar` from `/usr/share/khiops`, as well as the `/usr/share/khiops` directory itself

## Release Checklist Code
````md
 - **Preparation**
  - [ ] Create a `prepare-release-x.y.z` branch based on the `main` (or `main-v10`) branch, then check it out
  - [ ] Update the `README.txt` and `WHATSNEW.txt` in the `packaging/common/khiops` directory
  - [ ] Update the [PDF documentation][pdf-docs]
  - [ ] Update the technical repository [khiops-win-install-assets][win-install-assets]
    - Update the versions of all assets that require an update, namely:
      - Khiops PDF Docs
      - Java JRE installer
      - MS MPI installer
  - [ ] Update the Windows assets tag/version in the Windows installer workflow (`pack-nsis.yml`)
  - [ ] Update the Khiops version to `x.y.z-(a|b|rc).w` in the `src/Learning/KWUtils/KWKhiopsVersion.h` header file
  - [ ] Create a PR from the `prepare-release-x.y.z` branch and have it reviewed
  - [ ] Once the PR is approved, merge it into the `main` branch
  - [ ] Create the `x.y.z-(a|b|rc).w` pre-release tag on the `main` branch (see [Versioning](#versioning))
    - Example: `git tag 10.2.1-a.1 main`
- **Full Test Loop**
  - [ ] Push the `x.y.z-(a|b|rc).w` pre-release tag
    - Example: `git push origin tag 10.2.1-a.1`
    - This will create a pre-release entry in the [Releases section][khiops-releases] containing all CI/CD-generated packages.
  - [ ] Publish the Conda packages `x.y.z` to the `conda-forge` Conda channel. [See the dedicated Conda section](#publish-the-conda-packages-xyz-to-the-conda-forge-conda-channel)
  - [ ] Publish the pip packages to TestPyPI:
    - Run the manual "pip packaging" action. Select the current tag as the `branch` input and `testpypi` as the Python repository.
  - [ ] Test the conda packages with the manual action `Test Conda Packages`.
  - [ ] Run the end-to-end tests (also known as `LearningTest`) in all supported environments
    - If you find major issues, create issues and PRs to address them, then return to "Create pre-release tag..."
  - [ ] Execute the [manual tests for the GUI packages](#manual-testing-procedure)

- **Finalize the Release**
  - [ ] Update `src/Learning/KWUtils/KWKhiopsVersion.h` to the final release version, `x.y.z`
  - [ ] Create the final release tag (see also [Versioning](#versioning)):
      - Tag the `main` branch (`git tag x.y.z main` then `git push origin tag x.y.z`)
      - This will create an entry in the [Releases section][khiops-releases] containing all CI/CD-generated packages.
  - [ ] Sign the Windows installer:
    - Run the [Windows installer workflow][win-installer-wf] from the tag and select the "Sign installer and binaries" checkbox.
    - This will replace the Windows installer in the release with a signed version.
  - [ ] Copy the release notes from [packaging/common/khiops/WHATSNEW.txt](../../packaging/common/khiops/WHATSNEW.txt) to the new [GitHub release entry][khiops-releases].
  - [ ] Remove any packages that are not intended for release.
    - We create additional packages for testing purposes (for example, `kni-transfer`).
  - [ ] Publish the Conda packages `x.y.z` to the `conda-forge` Conda channel. [See the dedicated Conda section](#publish-the-conda-packages-xyz-to-the-conda-forge-conda-channel)
  - [ ] Publish the pip packages to PyPI with **signed** binaries:
    - Run the manual "pip packaging" action. Select the current tag as the `branch` input, `pypi` as the Python repository, and check the "Sign Windows binaries ..." box.
  - [ ] Publish the pip packages to TestPyPI to facilitate downstream CI/CD processes for enabler packaging and delivery:
    - Run the manual "pip packaging" action. Select the current tag as the `branch` input and `testpypi` as the Python repository.
  - [ ] Set the release as "latest" and unset its "pre-release" status.
  - [ ] Update the [KNI tutorial](https://github.com/KhiopsML/KNI-tutorial) repository by running the `update-kni-tutorial` workflow from the tag you just created.
  - [ ] Release the Linux packages on DCR with help from the GitLab repository `release_khiops_packages`.
  - [ ] Release the `khiops-python` enabler.
  - [ ] Release the `khiops-docker` enabler.
  - [ ] Release the `khiops-notebook` enabler.
    - **At this point the update of the website is unblocked**

### Publish the Conda packages `x.y.z` to the `conda-forge` Conda channel
  - [ ] Create a _personal_ fork of the GitHub repository `https://github.com/conda-forge/khiops-binaries-feedstock`.
  - [ ] Create a branch named `update-version-to-x-y-z` in your `khiops-binaries-feedstock` fork.
  - [ ] Update the following fields in `khiops-binaries-feedstock/recipe/meta.yaml` on the new branch:
    - Set `khiops_version` to `x.y.z`.
    - Set `source: sha256` to the SHA-256 checksum of the source tarball for the `x.y.z` GitHub release:
         ```bash
         curl -sL  https://github.com/KhiopsML/khiops/archive/refs/tags/x.y.z.tar.gz | openssl sha256 | awk '{print $NF}'
         ```
    - [ ] In `recipes/conda_build_config.yaml`, set the `channel_targets: - conda-forge <label>` value to `main` for official releases or `rc` for pre-releases.
    - [ ] Create a pull request from the new branch in the `khiops-binaries-feedstock` repository.
    - [ ] Request that the repository be re-rendered for the PR, following the instructions in the PR.

[khiops-releases]: https://github.com/KhiopsML/khiops/releases
[manual-tests]: https://github.com/KhiopsML/khiops/wiki/Release-Process#manual-testing-procedure
[pdf-docs]: https://github.com/khiopsml/khiops-pdf-docs
[win-install-assets]: https://github.com/khiopsml/khiops-win-install-assets
[win-installer-wf]: https://github.com/KhiopsML/khiops/actions/workflows/pack-nsis.yml
````

