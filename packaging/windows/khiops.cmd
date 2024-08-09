@echo off
setlocal

REM ========================================================
REM See the khiops_env script for full documentation on the
REM environment variables used by Khiops
REM ========================================================

REM ========================================================
REM Initialization of the installation directory of Khiops

REM Test is Khiops environment already set up
if "%KHIOPS_HOME%".=="". set KHIOPS_HOME=$_KhiopsHome
if not exist "%KHIOPS_HOME%\bin$_BinSuffix\$_ToolName.exe" goto ERR_PATH

REM Test if batch mode from parameters
set _KHIOPS_BATCH_MODE=
for %%i in (%*) do if %%i.==-b. set _KHIOPS_BATCH_MODE=true

REM Initialize Khiops env variables
call "%KHIOPS_HOME%\bin\khiops_env" --env > NUL

REM Set path
set path=%KHIOPS_PATH%;%KHIOPS_JAVA_PATH%;%path%
set classpath=%KHIOPS_CLASSPATH%;%classpath%

REM ========================================================
REM Start Khiops (with or without parameteres)

if %1.==. goto NOPARAMS
if not %1.==. goto PARAMS

REM Start without parameters
:NOPARAMS
if not exist "%KHIOPS_LAST_RUN_DIR%" md "%KHIOPS_LAST_RUN_DIR%"
if not exist "%KHIOPS_LAST_RUN_DIR%" goto PARAMS

%KHIOPS_MPI_COMMAND% "%KHIOPS_PATH%$_BinSuffix\$_ToolName" -o "%KHIOPS_LAST_RUN_DIR%\$_ScenarioFileName" -e "%KHIOPS_LAST_RUN_DIR%\$_LogFileName"

if %errorlevel% EQU 0 goto END
goto ERR_RETURN_CODE

REM Start with parameters
:PARAMS
%KHIOPS_MPI_COMMAND% "%KHIOPS_PATH%$_BinSuffix\$_ToolName" %*
if %errorlevel% EQU 0 goto END
goto ERR_RETURN_CODE

REM ========================================================
REM Error messages

:ERR_PATH
start "KHIOPS CONFIG PROBLEM" echo ERROR Incorrect installation directory for Khiops (File $_ToolName.exe not found in directory %KHIOPS_PATH%$_BinSuffix)
exit /b 1

:ERR_RETURN_CODE
start "KHIOPS EXECUTION PROBLEM" cmd /k "echo ERROR Khiops ended with return code %errorlevel% & echo Contents of the log file at %KHIOPS_LAST_RUN_DIR%\$_LogFileName: & type %KHIOPS_LAST_RUN_DIR%\$_LogFileName"
goto END

:END
endlocal

REM Return 1 if fatal error, 0 otherwise
exit /b %errorlevel%