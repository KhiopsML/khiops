@echo off
setlocal
REM Access to genere tool from all genereall.cmd files in the src tree
REM Allow to get to the last available compiled version of genere

REM This solution is temporary, waiting for a more professional solution using makefiles to genere source code from .dd files.

REM Search genere tool in release binaries
if exist %~dp0..\build\windows-msvc-release\bin\genere.exe goto GENERE
goto ERROR

:GENERE
%~dp0..\build\windows-msvc-release\bin\genere %*
goto END

:ERROR
echo Error : genere tool not found among binaries in build directory

:END
endlocal