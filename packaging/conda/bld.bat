REM Echo all output
@echo on

REM Build the Khiops binaries
REM Specify empty target platform and generator toolset for CMake with Ninja on
REM Windows
REM Ninja does not expect target platform and generator toolset.
REM However, CMake Windows presets set these, which results in Ninja failure.
cmake --preset windows-msvc-release -DBUILD_JARS=OFF -DTESTING=OFF -A "" -T ""
cmake --build --preset windows-msvc-release --parallel --target MODL MODL_Coclustering KhiopsNativeInterface KNITransfer 

REM Copy the MODL binaries to the Conda PREFIX path
mkdir %PREFIX%\bin
copy build\windows-msvc-release\bin\MODL.exe %PREFIX%\bin
copy build\windows-msvc-release\bin\MODL_Coclustering.exe %PREFIX%\bin
copy build\windows-msvc-release\bin\KNITransfer.exe %PREFIX%\bin
copy build\windows-msvc-release\bin\KhiopsNativeInterface.dll %PREFIX%\lib

if errorlevel 1 exit 1
