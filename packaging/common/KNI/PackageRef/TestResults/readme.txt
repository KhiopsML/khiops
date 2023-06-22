Khiops Native Interface 7.5

Test C source
==============

Files
-----

readme.txt: this file

KNIRecodeFile.c: 
KNITest.c: 
KNITest.h: 
	C source code using KNI functions

makefile:
	Sample makefile using the  gcc compiler

KNITest.exe: 
	Windows 32 bit executable resulting from compilation of source code

StartKNITest.cmd:
	Sample use of executable


Executable
----------
KNIRecodeFile.exe
  Parameters:
	Dictionary file
	Dictionary
	Input file
	Output file

Compile
-------

The C source code must be compiled, using the KNI include and librairy files.
For example, the following gcc comand line compiles for the windows 32 bits platform.

See makefile for an example using the gcc compiler, and type make to compile.


Execute
-------

StartKNITest.cmd starts the test and store the the results in results.txt
