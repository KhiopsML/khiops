REM Echo all output
@echo on

REM Build the Khiops binaries
cmake --preset windows-msvc-release -DBUILD_JARS=OFF -DTESTING=OFF
cmake --build --preset windows-msvc-release --parallel --target MODL MODL_Coclustering KhiopsNativeInterface KNITransfer 

REM Copy the MODL binaries to the Conda PREFIX path
mkdir %PREFIX%\bin
copy build\windows-msvc-release\bin\MODL.exe %PREFIX%\bin
copy build\windows-msvc-release\bin\MODL_Coclustering.exe %PREFIX%\bin
copy build\windows-msvc-release\bin\KNITransfer.exe %PREFIX%\bin
copy build\windows-msvc-release\bin\KhiopsNativeInterface.dll %PREFIX%\lib

if errorlevel 1 exit 1
