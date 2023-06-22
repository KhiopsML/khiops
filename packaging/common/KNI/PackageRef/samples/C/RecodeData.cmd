@echo off
setlocal
REM To get the KNI DLL in the path
set path=..\..\bin;%path%

KNIRecodeFile ..\data\ModelingIris.kdic SNB_Iris  ..\data\Iris.txt ..\data\R_Iris.txt > ..\data\test.log

KNIRecodeMTFiles -d ..\data\ModelingSpliceJunction.kdic SNB_SpliceJunction -i ..\data\SpliceJunction.txt 1 -s DNA ..\data\SpliceJunctionDNA.txt 1 -o ..\data\R_SpliceJunction.txt > ..\data\testMT.log 2>&1

endlocal