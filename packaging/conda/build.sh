#!/bin/bash

# Set-up the shell to behave more like a general-purpose programming language
set -euo pipefail

# Choose the build preset for macOS/Linux
if [[ "$(uname)" == "Darwin" ]]
then
  CMAKE_PRESET="macos-clang-release"
else
  CMAKE_PRESET="linux-gcc-release"
fi

# Build MODL and MODL_Coclustering
cmake --fresh --preset $CMAKE_PRESET -DBUILD_JARS=OFF -DTESTING=OFF
cmake --build --preset $CMAKE_PRESET --parallel --target MODL MODL_Coclustering

# Copy the MODL binaries to the Conda PREFIX path
cp "./build/$CMAKE_PRESET/bin/MODL" "$PREFIX/bin"
cp "./build/$CMAKE_PRESET/bin/MODL_Coclustering" "$PREFIX/bin"

# Custom rpath relocation and signing executables for macOS in arm64
#
# In osx-arm64 executing any binary that is not signed will make appear popups appearing demanding
# "accepting incoming connections". Since our application doesn't need any connections from the
# outside the machine this doesn't affect the execution but since it is launched with MPI the number
# of popups appearing is high. This is difficult to fix for the user because the if the artifact is
# not signed it will reappear even if we click in the "Allow" button. So we sign the MODL
# executables to solve this (only a single popup concerning mpiexec.hydra may appear but for this
# application pressing on "Allow" works).
#
# However, in the default settings, `conda build` relocalizes the executable by changing rpath of
# the library paths at $PREFIX by relative ones and in doing so it nullifies any signature. So we
# do ourselves this procedure first and then sign the binary.
#
# Note that in meta.yaml for osx-arm64 we have custom build.binary_relocation and
# build.detect_binary_files_with_prefix option
#
# This part must be executed in a root machine to be non-interactive (eg. GitHub runner)
# It also needs the following environment variable:
# - KHIOPS_APPLE_CERTIFICATE_COMMON_NAME: The second column of the `security find-identity` command
# A base64 encoded certificate may also be provided, the following 2 variables must be set
# - KHIOPS_APPLE_CERTIFICATE_BASE64: The identity file .p12 (certificate + private key) in base64
# - KHIOPS_APPLE_CERTIFICATE_PASSWORD: Password for the certificate file
# - KHIOPS_APPLE_TMP_KEYCHAIN_PASSWORD: A temporary password to decrypt the certificate
#
cd ..
if [[ "$(uname)" == "Darwin" && -n "${KHIOPS_APPLE_CERTIFICATE_COMMON_NAME-}" ]]
then
  # Inform about the signature process
  echo "Signing binaries with the certificate named '${KHIOPS_APPLE_CERTIFICATE_COMMON_NAME}'"

  # Delete the rpath of each executable
  # Delete two times for MODL because for some reason it is there 2 times
  install_name_tool -delete_rpath "$PREFIX/lib" "$PREFIX/bin/MODL"
  install_name_tool -delete_rpath "$PREFIX/lib" "$PREFIX/bin/MODL"
  install_name_tool -delete_rpath "$PREFIX/lib" "$PREFIX/bin/MODL_Coclustering"

  # Add the relative rpath as conda build would
  install_name_tool -add_rpath "@loader_path/../lib" "$PREFIX/bin/MODL"
  install_name_tool -add_rpath "@loader_path/../lib" "$PREFIX/bin/MODL_Coclustering"

  if [[ -n "${KHIOPS_APPLE_CERTIFICATE_BASE64-}" ]]
  then
    # Keychain setup slightly modified from: https://stackoverflow.com/a/68577995
    # Before importing identity
    # - Set the default user login keychain
    # - Create a temporary keychain
    # - Append temporary keychain to the user domain
    # - Remove relock timeout
    # - Unlock the temporary keychain
    sudo security list-keychains -d user -s login.keychain
    sudo security create-keychain -p "$KHIOPS_APPLE_TMP_KEYCHAIN_PASSWORD" kh-tmp.keychain
    sudo security list-keychains -d user -s kh-tmp.keychain \
      "$(security list-keychains -d user | sed s/\"//g)"
    sudo security set-keychain-settings kh-tmp.keychain
    sudo security unlock-keychain -p "$KHIOPS_APPLE_TMP_KEYCHAIN_PASSWORD" kh-tmp.keychain

    # Add identity (certificate + private key) to keychain
    echo "$KHIOPS_APPLE_CERTIFICATE_BASE64" \
      | base64 --decode -i - -o kh-cert.p12
    sudo security import kh-cert.p12 \
      -k kh-tmp.keychain \
      -P "$KHIOPS_APPLE_CERTIFICATE_PASSWORD" \
      -A -T "/usr/bin/codesign"
    rm -f kh-cert.p12

    # Enable codesigning from a non user interactive shell
    sudo security set-key-partition-list -S apple-tool:,apple:, \
      -s -k "$KHIOPS_APPLE_TMP_KEYCHAIN_PASSWORD" \
      -D "$KHIOPS_APPLE_CERTIFICATE_COMMON_NAME" \
      -t private kh-tmp.keychain
  fi

  # We make sure to use the default macOS/Xcode codesign tool. This is because the sigtool python
  # package (installed by conda build as a dependency) makes an alias "codesign" which is prioritary
  # in the build environment. The alias, however, alias doesn't support signing with a proper
  # identity and makes the build fail!
  CODESIGN="/usr/bin/codesign"

  # Sign the MODL executable and check
  $CODESIGN --force --sign "$KHIOPS_APPLE_CERTIFICATE_COMMON_NAME" "$PREFIX/bin/MODL"
  $CODESIGN --force --sign "$KHIOPS_APPLE_CERTIFICATE_COMMON_NAME" "$PREFIX/bin/MODL_Coclustering"
  $CODESIGN -d -vvv "$PREFIX/bin/MODL"
  $CODESIGN -d -vvv "$PREFIX/bin/MODL_Coclustering"

  # Remove the temporary keychain and restore the login keychain as default if created
  if [[ -n "${KHIOPS_APPLE_CERTIFICATE_BASE64-}" ]]
  then
    sudo security delete-keychain kh-tmp.keychain
    sudo security list-keychains -d user -s login.keychain
  fi
fi
