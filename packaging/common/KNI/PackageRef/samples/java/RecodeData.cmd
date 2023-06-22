@echo off
setlocal

REM Home of java SDK; update according to local config
set JAVA_HOME=C:\Program Files\Java\jdk1.7.0_51\bin

REM To get the KNI DLL in the path
set path=..\..\bin;%path%

"%JAVA_HOME%\java" -d64 -cp kni.jar;jna.jar KNIRecodeFile ..\data\ModelingIris.kdic SNB_Iris  ..\data\Iris.txt ..\data\R_Iris.txt > ..\data\test.log 2>&1

endlocal

