# Khiops Conda Packaging Scripts

## How to Build
You'll need `conda-build` installed in your system.

```bash
# At the root of the repo
# These commands will leave a ready to use conda channel in `./khiops-conda`

# khiops version must be set before launching the build (can be retrieved with the script scripts/khiops-version).
# The dash must be removed from the version to be compliant with the conda versionning policy
export KHIOPS_RAW_VERSION=$(scripts/khiops-version)
export KHIOPS_VERSION=$(echo $KHIOPS_RAW_VERSION | sed 's/-//')

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