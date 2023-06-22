REM Compilation of KNIRecodeFile sample
REM Compiler: Microsoft Visual Studio
REM os=windows
REM Platform=64 bits

REM Set Microsoft Visual Studio environement 2019 (change in case of another compiler)
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall" x64

cl KNIRecodeFile.c KhiopsNativeInterface64.lib -I ..\..\include /link "/LIBPATH:../../lib"

cl KNIRecodeMTFiles.c KhiopsNativeInterface64.lib -I ..\..\include /link "/LIBPATH:../../lib"

del *.obj
