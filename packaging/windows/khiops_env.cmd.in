@echo off

if %1.==--env. goto DISPLAY_ENV
if %*.==. goto SET_ENV

:HELP
echo Usage: khiops_env [-h, --help] [--env]
echo khiops_env is an internal script intended to be used by Khiops tool and Khiops wrappers only.
echo It sets all the environment variables required by the Khiops to run correctly (Java, MPI, etc).
echo Options:
echo    -h, --help show this help message and exit
echo    -env show the environment list and exit
echo.
echo The following variables are used to set the path and classpath for the prerequisite of Khiops.
echo.
echo KHIOPS_PATH: full path of Khiops executable
echo KHIOPS_COCLUSTERING_PATH: full path of Khiops coclustering executable
echo KHIOPS_MPI_COMMAND: MPI command to call the Khiops tool
echo KHIOPS_JAVA_PATH: path of Java tool, to add in path
echo KHIOPS_CLASSPATH: Khiops java libraries, to add in classpath
echo KHIOPS_DRIVERS_PATH: search path of the drivers (by default Khiops bin directory if not defined)
echo.
echo If they are not already defined, the following variables used by Khiops are set:
echo.
echo KHIOPS_LAST_RUN_DIR: directory where Khiops writes output command file and log
echo   (when not defined with -e and -o)
echo KHIOPS_PROC_NUMBER: processes number launched by Khiops (it's default value corresponds to the
echo   number of physical cores of the computer)
echo.
echo The following variables are not defined by default and can be used to change some default
echo  properties of Khiops:
echo.
echo KHIOPS_TMP_DIR: Khiops temporary directory location (default: the system default).
echo   This location can be modified from the tool as well.
echo KHIOPS_MEMORY_LIMIT: Khiops memory limit in MB (default: the system memory limit).
echo   The minimum value is 100 MB; this setting is ignored if it is above the system's memory limit.
echo   It can only be reduced from the tool.
echo KHIOPS_API_MODE: standard or api mode for the management of output result files created by Khiops
echo   In standard mode, the result files are stored in the train database directory,
echo   unless an absolute path is specified, and the file extension is forced if necessary.
echo   In api mode, the result files are stored in the current working directory, using the specified results as is.
echo   . default behavior if not set: standard mode
echo   . set to 'false' to force standard mode
echo   . set to 'true' to force api mode
echo KHIOPS_RAW_GUI: graphical user interface for file name selection
echo   . default behavior if not set: depending on the file drivers available for Khiops
echo   . set to 'true' to allow file name selection with uri schemas
echo   . set to 'false' to allow local file name selection only with a file selection dialog
echo.
echo In case of configuration problems, the variables KHIOPS_JAVA_ERROR and KHIOPS_MPI_ERROR contain error messages.

if not %2.==. exit /b 1
if %1.==-h. exit /b 0
if %1.==--help. exit /b 0
exit /b 1

REM Set Khiops environment variables
:DISPLAY_ENV
setlocal
set DISPLAY_ENV=true

:SET_ENV
REM Initialize exported variables
set "KHIOPS_PATH="
set "KHIOPS_COCLUSTERING_PATH="
set "KHIOPS_MPI_COMMAND="
set "KHIOPS_JAVA_PATH="
set "KHIOPS_CLASSPATH="
set "KHIOPS_JAVA_ERROR="
set "KHIOPS_MPI_ERROR="

REM Set Khiops home to parent directory
for %%a in ("%~dp0..") do set "_KHIOPS_HOME=%%~fa"

REM KHIOPS_PATH
set "KHIOPS_PATH=%_KHIOPS_HOME%\bin\MODL.exe"
set "KHIOPS_COCLUSTERING_PATH=%_KHIOPS_HOME%\bin\MODL_Coclustering.exe"

REM KHIOPS_LAST_RUN_DIR
if "%KHIOPS_LAST_RUN_DIR%". == "". set "KHIOPS_LAST_RUN_DIR=%USERPROFILE%\khiops_data\lastrun"

REM KHIOPS_PROC_NUMBER
if "%KHIOPS_PROC_NUMBER%". == "". for /f %%i in ('"%~dp0_khiopsgetprocnumber"') do set "KHIOPS_PROC_NUMBER=%%i"
if "%KHIOPS_PROC_NUMBER%". == "". set "KHIOPS_PROC_NUMBER=1"

REM Set MPI binary (mpiexec)
if %KHIOPS_PROC_NUMBER% LEQ 2 goto MPI_DONE
goto @SET_MPI@

:MPI_PARAMS
REM Add the MPI parameters
if not "%KHIOPS_MPI_COMMAND%." == "." set "KHIOPS_MPI_COMMAND="%KHIOPS_MPI_COMMAND%" -n %KHIOPS_PROC_NUMBER%"
:MPI_DONE

set _KHIOPS_GUI=@GUI_STATUS@
if "%_KHIOPS_GUI%" == "false" GOTO SKIP_GUI

REM Set Java environment
set _JAVA_ERROR=false
if not exist "%_KHIOPS_HOME%\jre\bin\server\" set _JAVA_ERROR=true
if not exist "%_KHIOPS_HOME%\jre\bin\" set _JAVA_ERROR=true

if  "%_JAVA_ERROR%" == "false" (
    set "KHIOPS_JAVA_PATH=%_KHIOPS_HOME%\jre\bin\server\;%_KHIOPS_HOME%\jre\bin\"
) else set "KHIOPS_JAVA_ERROR=The JRE is missing in Khiops home directory, please reinstall Khiops"

REM KHIOPS_CLASSPATH
set "KHIOPS_CLASSPATH=%_KHIOPS_HOME%\bin\norm.jar"
set "KHIOPS_CLASSPATH=%_KHIOPS_HOME%\bin\khiops.jar;%KHIOPS_CLASSPATH%"

:SKIP_GUI

@IS_CONDA_VAR@

@SET_KHIOPS_DRIVERS_PATH@

REM unset local variables
set "_KHIOPS_GUI="
set "_JAVA_ERROR="
set "_KHIOPS_HOME="

if not "%DISPLAY_ENV%" == "true" exit /b 0

REM Print the environment list on the standard output
echo KHIOPS_PATH %KHIOPS_PATH%
echo KHIOPS_COCLUSTERING_PATH %KHIOPS_COCLUSTERING_PATH%
echo KHIOPS_MPI_COMMAND %KHIOPS_MPI_COMMAND%
echo KHIOPS_JAVA_PATH %KHIOPS_JAVA_PATH%
echo KHIOPS_CLASSPATH %KHIOPS_CLASSPATH%
echo KHIOPS_LAST_RUN_DIR %KHIOPS_LAST_RUN_DIR%
echo KHIOPS_PROC_NUMBER %KHIOPS_PROC_NUMBER%
echo KHIOPS_TMP_DIR %KHIOPS_TMP_DIR%
echo KHIOPS_MEMORY_LIMIT %KHIOPS_MEMORY_LIMIT%
echo KHIOPS_API_MODE %KHIOPS_API_MODE%
echo KHIOPS_RAW_GUI %KHIOPS_RAW_GUI%
echo KHIOPS_DRIVERS_PATH %KHIOPS_DRIVERS_PATH%
echo KHIOPS_JAVA_ERROR %KHIOPS_JAVA_ERROR%
echo KHIOPS_MPI_ERROR %KHIOPS_MPI_ERROR%
endlocal
exit /b 0

REM Set mpiexec path for conda installation
:SET_MPI_CONDA
set "KHIOPS_MPI_COMMAND=%_KHIOPS_HOME%\Library\bin\mpiexec.exe"
if not exist "%KHIOPS_MPI_COMMAND%" goto ERR_MPI
goto MPI_PARAMS

REM Set mpiexec path for system wide installation
:SET_MPI_SYSTEM_WIDE
REM ... with the standard variable MSMPI_BIN
set "KHIOPS_MPI_COMMAND=%MSMPI_BIN%mpiexec.exe"
if  exist "%KHIOPS_MPI_COMMAND%" goto MPI_PARAMS
REM ... if MSMPI_BIN is not correctly defined 
REM ... we try to call directly mpiexec (assuming its path is in the 'path' variable)
set "KHIOPS_MPI_COMMAND=mpiexec"
where /q "%KHIOPS_MPI_COMMAND%"
if ERRORLEVEL 1 goto ERR_MPI
REM ... finally we check if it is the good MPI implementation: "Microsoft MPI"
"%KHIOPS_MPI_COMMAND%" | findstr /c:"Microsoft MPI" > nul
if ERRORLEVEL 1 goto ERR_MPI_IMPL
goto MPI_PARAMS


:ERR_MPI
set "KHIOPS_MPI_ERROR=We didn't find mpiexec in the regular path. Parallel computation is unavailable: Khiops is launched in serial"
set "KHIOPS_MPI_COMMAND="
goto MPI_DONE

:ERR_MPI_IMPL
set "KHIOPS_MPI_ERROR=We can't find the right implementation of mpiexec, we expect to find Microsoft MPI. Parallel computation is unavailable: Khiops is launched in serial"
set "KHIOPS_MPI_COMMAND="
goto MPI_DONE