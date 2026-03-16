# Khiops Conda Packaging Scripts

The conda packages are built on conda-forge using the recipe from the GitHub repository [khiops-binaries-feedstock](https://github.com/conda-forge/khiops-binaries-feedstock).

> [!WARNING]
> The scripts in the current repository enable the creation of test or POC packages of Khiops and KNI. Production-ready packages are built on conda-forge.

## How to Build
We need `conda-build` installed in the system.

We need to make sure that Conda is configured to use conda-forge as its default channel and that the vanilla default channel (defaults) is removed, e.g. by writing:

```bash
$ conda config --add channels conda-forge
$ conda config --remove channels defaults
```

Or if we want to keep the vanilla defaults channel, we could give the priority to conda-forge:

```bash
$ conda config --add channels conda-forge
$ conda config --set channel_priority strict
```

Thus, the user's $HOME/.condarc file would be updated accordingly and --channel conda-forge would no longer be needed.

To build `khiops-core` package, we need to run these commands at the root of the repo (it will leave a ready to use conda channel in `./khiops-conda`):

```bash
# At the root of the repo

# khiops version must be set before launching the build (can be retrieved with the script scripts/khiops-version).
export KHIOPS_VERSION=$(scripts/khiops-version)

conda build --output-folder ./khiops-conda packaging/conda
```

> [!NOTE]
> Don't forget to set KHIOPS_VERSION before calling `conda build`.