@echo off
REM Start a shell with the LearningTestTool command files in the PATH
REM Usage: start-learningtest.cmd [starting-directory]
REM If no directory is provided, starts in ..\LearningTest relative to the script.

REM Add the script's cmd directory to the PATH
set "path=%~dp0cmd;%path%"

REM Open a new window (title = script name) and set the working directory without creating START_DIR
if "%~1"=="" (
  start "%~n0" /D "%~dp0..\LearningTest" cmd
) else (
  start "%~n0" /D "%~1" cmd
)
