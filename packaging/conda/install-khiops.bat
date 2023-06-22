REM Echo all output
@echo on

REM Copy the MODL binaries to the anaconda PREFIX path
mkdir %PREFIX%\bin
copy build\windows-msvc-release\bin\MODL.exe %PREFIX%\bin
copy build\windows-msvc-release\bin\MODL_Coclustering.exe %PREFIX%\bin
