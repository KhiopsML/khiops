#echo off
REM Start dos shell with the LearningTestTool command files in the path

REM Add LearningTest script cmd dir in the path
set path=%~dp0cmd;%path%

REM Open a new shell with given title and starting directory
start "%~n0" /D "%~dp0..\LearningTest"
