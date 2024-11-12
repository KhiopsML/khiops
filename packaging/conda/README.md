# Khiops Conda Packaging Scripts

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

In the CI this is ensured by the usage of miniforge-version in the invocation of the setup-miniconda action.

To build `khiops-core` package, we need to run these commands at the root of the repo (it will leave a ready to use conda channel in `./khiops-conda`):

```bash
# At the root of the repo

# khiops version must be set before launching the build (can be retrieved with the script scripts/khiops-version).
export KHIOPS_VERSION=$(scripts/khiops-version)

conda build --output-folder ./khiops-conda packaging/conda
```

### Signing the Executables in macOS
The script can sign the Khiops binaries. This is to avoid annoying firewall pop-ups. To enable this
set the following environment variable:
- `KHIOPS_APPLE_CERTIFICATE_COMMON_NAME`: The common name of the signing certificate.

If the certificate is available in your keychain then the `conda build` command will use it to sign
the `MODL*` binaries. You may have to input your password.

Alternatively, a certificate file encoded in base64 may be provided by additionally setting the
following environment variables:
- `KHIOPS_APPLE_CERTIFICATE_BASE64`: The base64 encoding of the signing certificate.
- `KHIOPS_APPLE_CERTIFICATE_PASSWORD`: The password of the signing certificate.
- `KHIOPS_APPLE_TMP_KEYCHAIN_PASSWORD` : A password for the temporary keychain created in the process.

If the process is executed as root (eg. Github Runner) then there is no need to input a password to
access the keychain. Otherwise you'll be prompt with it.

For more details see the comments in the signing section of `build.sh`.