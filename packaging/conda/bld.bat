REM Echo all output
@echo on

REM Build the Khiops binaries
REM Specify empty target platform and generator toolset for CMake with Ninja on
REM Windows
REM Ninja does not expect target platform and generator toolset.
REM However, CMake Windows presets set these, which results in Ninja failure.
cmake --preset windows-msvc-release -DBUILD_JARS=OFF -DTESTING=OFF -A "" -T ""
cmake --build --preset windows-msvc-release --parallel --target MODL MODL_Coclustering KhiopsNativeInterface KNITransfer _khiopsgetprocnumber

mkdir %PREFIX%\bin

REM Copy the khiops-core binaries to the Conda PREFIX path: MODL, MODL_Cocluetsring and _khiopsgetprocnumber.
REM This last one is used by khiops_env to get the physical cores number 
copy build\windows-msvc-release\bin\MODL.exe %PREFIX%\bin
copy build\windows-msvc-release\bin\MODL_Coclustering.exe %PREFIX%\bin
copy build\windows-msvc-release\bin\_khiopsgetprocnumber.exe %PREFIX%\bin

REM Copy the KNITransfer for the kni-transfer package (a test package for kni)
copy build\windows-msvc-release\bin\KNITransfer.exe %PREFIX%\bin

REM Copy the KhiopsNativeInterface libs for the kni package
copy build\windows-msvc-release\bin\KhiopsNativeInterface.dll %PREFIX%\bin
copy build\windows-msvc-release\lib\KhiopsNativeInterface.lib %PREFIX%\lib

REM Copy the scripts to the Conda PREFIX path
copy build\windows-msvc-release\tmp\khiops_env.cmd %PREFIX%\bin
copy packaging\windows\khiops_coclustering.cmd %PREFIX%\bin
copy packaging\windows\khiops.cmd %PREFIX%\bin

if errorlevel 1 exit 1
