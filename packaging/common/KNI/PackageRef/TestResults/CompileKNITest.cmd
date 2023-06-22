REM Compilation of KNIRecodeFile sample
REM Compiler: Microsoft Visual Studio
REM os=windows
REM Platform=64 bits

REM Set Microsoft Visual Studio environement 2019 (change in case of another compiler)
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall" x64

cl KNITest.c KNIRecodeFile.c KNIRecodeMTFiles.c KhiopsNativeInterface64.lib -I ..\..\KNI\include /link "/LIBPATH:..\..\KNI\lib"
	
del *.obj
