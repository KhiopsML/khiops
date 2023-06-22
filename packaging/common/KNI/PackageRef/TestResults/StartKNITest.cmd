@echo off
setlocal
set path=..\..\KNI\bin;%path%

KNITest ..\samples\data\ModelingIris.kdic SNB_Iris  ..\samples\data\Iris.txt R_Iris.txt KNIErrors.log > CompleteTest.log

endlocal