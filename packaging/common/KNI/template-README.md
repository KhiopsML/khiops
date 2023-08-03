
# Khiops Native Interface examples @PROJECT_VERSION@

The purpose of Khiops Native Interface (KNI) is to allow a deeper integration of Khiops in information systems, by mean of the C programming language, using a dynamic link library (DLL). This relates specially to the problem of model deployment, which otherwise requires the use of input and output data files when using directly the Khiops tool in batch mode. See Khiops Guide for an introduction to dictionary files, dictionaries, database files and deployment.

The Khiops deployment features are thus made public through an API with a DLL. Therefore, a Khiops model can be deployed directly from any programming language, such as C, C++, Java, Python, Matlab, etc. This enables real time model deployment without the overhead of temporary data files or launching executables. This is critical for certain applications in marketing or targeted advertising on the web.

All KNI functions are C functions for easy use with other programming languages. They return a positive or null value in case of success, and a negative error code in case of failure.

See [KhiopsNativeInterface.h](include/KhiopsNativeInterface.h) for a detailed description of KNI functions.

> [!IMPORTANT]
> The functions are not reentrant (thread-safe): the DLL can be used simultaneously by several executables, but not simultaneously by several threads in the same executable.




# KNI installation

On windows, download and extract the zip file to your machine. Set the environment variable `KNI_HOME` to the extracted directory. This variable is used in the following examples.

On linux TODO

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
gcc -o KNIRecodeFile cpp/KNIRecodeFile.c -lKhiopsNativeInterface -ldl
gcc -o KNIRecodeMTFiles cpp/KNIRecodeMTFiles.c -lKhiopsNativeInterface -ldl
```

On windows, open a "Visual Studio Developer Console" and run:

```cmd
cl KNITest.c KNIRecodeFile.c KNIRecodeMTFiles.c %KNI_HOME%\lib\KhiopsNativeInterface64.lib \
    -I %KNI_HOME%\include /link "/LIBPATH:%KNI_HOME%\bin"
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
javac -cp jna.jar KNIRecodeFile.java KNI.java
jar cf kni.jar *.class
```

## Launch

Recodes the iris dataset from the data directory using the SNB_Iris dictionary.

On Linux:

```bash
java -d64 -cp kni.jar;jna.jar KNIRecodeFile data/ModelingIris.kdic SNB_Iris \
      data/Iris.txt data/R_Iris.txt > data/test.log 2>&1
```

On Windows:

```cmd
java -d64 -cp kni.jar:jna.jar KNIRecodeFile data/ModelingIris.kdic SNB_Iris \
      data/Iris.txt data/R_Iris.txt > data/test.log 2>&1
```
