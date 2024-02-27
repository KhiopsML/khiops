
# Khiops Native Interface  v@PROJECT_VERSION@

This project provides all the basics to use the Khiops Native Interface (KNI): installation and examples.

The purpose of KNI is to allow a deeper integration of Khiops in information systems, by mean of the C programming language, using a dynamic link library (DLL). This relates specially to the problem of model deployment, which otherwise requires the use of input and output data files when using directly the Khiops tool in batch mode. See Khiops Guide for an introduction to dictionary files, dictionaries, database files and deployment.

The Khiops deployment features are thus made public through an API with a DLL. Therefore, a Khiops model can be deployed directly from any programming language, such as C, C++, Java, Python, Matlab, etc. This enables real time model deployment without the overhead of temporary data files or launching executables. This is critical for certain applications in marketing or targeted advertising on the web.

All KNI functions are C functions for easy use with other programming languages. They return a positive or null value in case of success, and a negative error code in case of failure.

See [KhiopsNativeInterface.h](include/KhiopsNativeInterface.h) for a detailed description of KNI functions.

> [!CAUTION]
> The functions are not reentrant (thread-safe): the DLL can be used simultaneously by several executables, but not simultaneously by several threads in the same executable.




# KNI installation

## Windows

Download [KNI-@PROJECT_VERSION@.zip](https://github.com/KhiopsML/khiops/releases/tag/@PROJECT_VERSION@/KNI-@PROJECT_VERSION@.zip) and extract it to your machine. Set the environment variable `KNI_HOME` to the extracted directory. This variable is used in the following examples.

## Linux

On linux, go to the [release page](https://github.com/KhiopsML/khiops/releases/tag/@PROJECT_VERSION@/) and download the kni package. The name of the package begins with **kni** and ends with the **code name** of the OS. The code name is in the release file of the distribution (here, it is jammy):
```bash
$ cat /etc/os-release
PRETTY_NAME="Ubuntu 22.04.4 LTS"
NAME="Ubuntu"
VERSION_ID="22.04"
VERSION="22.04.4 LTS (Jammy Jellyfish)"
VERSION_CODENAME=jammy
ID=ubuntu
ID_LIKE=debian
HOME_URL="https://www.ubuntu.com/"
SUPPORT_URL="https://help.ubuntu.com/"
BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
UBUNTU_CODENAME=jammy
```
Download the package according to the code name of your OS and install it with dpkg or yum:
- on debian-like distros: `sudo dpkg -i kni*.deb`
- on fedora-like distros: `sudo yum localinstall kni*.rpm`

# Application examples

Both examples in C and Java produce a sample binary `KNIRecodeFile`. It recodes an input file to an output file, using a Khiops dictionary from a dictionary file.

```bash
KNIRecodeFile <Dictionary file> <Dictionary> <Input File> <Output File> [Error file]
# The input file must have a header line, describing the structure of all its instances.
# The input and output files have a tabular format.
# The error file may be useful for debugging purposes. It is optional and may be empty.
```

A more complex example (available only in C) is `KNIRecodeMTFiles`, it recodes a set of multi-tables input files to an output file.

```bash
KNIRecodeMTFiles
  -d: <input dictionary file> <input dictionary>
  [-f: <field separator>
  -i: <input file name> [<key index>...]
  -s: <secondary data path> < file name> <key index>...
  -x: <external data root> <external data path> <external file name>
  -o: <output file name>
  [-e: <error file name>]
  [-m: <max memory (MB)>]
```

# Example with C

The files are located in [cpp directory](cpp/). They allow to build `KNIRecodeFile` and `KNIRecodeMTFiles`.

## Building the examples

On linux:

```bash
@BUILD_C_LINUX@
```

On windows, open a "Visual Studio Developer Console" and run:

```cmd
@BUILD_C_WINDOWS@
```

## Launch

Recode the iris dataset from the data directory using the SNB_Iris dictionary.

```bash
KNIRecodeFile data/ModelingIris.kdic SNB_Iris data/Iris.txt R_Iris.txt
```

Recode the splice junction multi-table dataset using the SNB_SpliceJunction dictionary.

```bash
KNIRecodeMTFiles -d data/ModelingSpliceJunction.kdic SNB_SpliceJunction -i .data/SpliceJunction.txt 1 \
    -s DNA  data/SpliceJunctionDNA.txt 1 -o R_SpliceJunction.txt
```

# Example with Java

The files are located in [java directory](java/). They allow to build `KNIRecodeFile.jar`. This example use [JNA](https://github.com/twall/jna#readme) to make calls to KhiopsNativeInterface.so/dll from Java.

## Building the examples

To compile Java files and create kni.jar file:

```bash
@BUILD_JAVA@
```

## Launch

Recodes the iris dataset from the data directory using the SNB_Iris dictionary.

On Linux:

```bash
@RUN_JAVA_LINUX@
```

On Windows:

```bash
@RUN_JAVA_WINDOWS@
```
