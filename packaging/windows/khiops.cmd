@echo off
setlocal

REM ========================================================
REM See the khiops_env script for full documentation on the
REM environment variables used by Khiops
REM ========================================================

REM ========================================================
REM Initialization of the installation directory of Khiops

REM Test is khiops_env is present
if not exist "%~dp0khiops_env.cmd" goto ERR_PATH_1

REM Initialize Khiops env variables
call "%~dp0khiops_env"

REM Test is Khiops environment already set up
if not exist "%KHIOPS_PATH%" goto ERR_PATH_2

REM display mpi configuration problems if any
if not "%KHIOPS_MPI_ERROR%". == "". echo %KHIOPS_MPI_ERROR%

REM Test if batch mode from parameters
set KHIOPS_BATCH_MODE=false
for %%i in (%*) do (
    for %%f in ("-h" "-b" "-s" "-v") do if /I "%%~i"=="%%~f" (
        set KHIOPS_BATCH_MODE=true
        goto BREAK_LOOP
    ) 
)
:BREAK_LOOP

if "%KHIOPS_BATCH_MODE%" == "true" if not "%KHIOPS_JAVA_ERROR%". == "". goto ERR_JAVA 
if "%_IS_CONDA%" == "true" if not "%KHIOPS_BATCH_MODE%" == "true" goto ERR_CONDA

REM Set path
set path=%~dp0;%KHIOPS_JAVA_PATH%;%path%
set classpath=%KHIOPS_CLASSPATH%;%classpath%

REM unset local variables
set "KHIOPS_BATCH_MODE="
set "_IS_CONDA="

REM ========================================================
REM Start Khiops (with or without parameteres)

if %1.==. goto NOPARAMS
if not %1.==. goto PARAMS

REM Start without parameters
:NOPARAMS
if not exist "%KHIOPS_LAST_RUN_DIR%" md "%KHIOPS_LAST_RUN_DIR%"
if not exist "%KHIOPS_LAST_RUN_DIR%" goto PARAMS

%KHIOPS_MPI_COMMAND% "%KHIOPS_PATH%" -o "%KHIOPS_LAST_RUN_DIR%\scenario._kh" -e "%KHIOPS_LAST_RUN_DIR%\log.txt"
if %errorlevel% EQU 0 goto END
goto ERR_RETURN_CODE

REM Start with parameters
:PARAMS
%KHIOPS_MPI_COMMAND% "%KHIOPS_PATH%" %*
if %errorlevel% EQU 0 goto END
goto ERR_RETURN_CODE

REM ========================================================
REM Error messages

:ERR_PATH_1
start "KHIOPS CONFIGURATION PROBLEM" echo ERROR "khiops_env.cmd is missing in directory %~dp0"
exit /b 1

:ERR_PATH_2
start "KHIOPS CONFIGURATION PROBLEM" echo ERROR "Incorrect installation directory for Khiops (File %KHIOPS_PATH% not found)"
exit /b 1

:ERR_RETURN_CODE
start "KHIOPS EXECUTION PROBLEM" cmd /k "echo ERROR Khiops ended with return code %errorlevel% & echo Contents of the log file at %KHIOPS_LAST_RUN_DIR%\log.txt: & type %KHIOPS_LAST_RUN_DIR%\log.txt"
goto END

:ERR_JAVA
start "KHIOPS CONFIGURATION PROBLEM" echo ERROR "%KHIOPS_JAVA_ERROR%"
exit /b 1

:ERR_CONDA
echo GUI is not available, please use the '-b' flag
exit /b 1

:END
endlocal

REM Return 1 if fatal error, 0 otherwise
exit /b %errorlevel%