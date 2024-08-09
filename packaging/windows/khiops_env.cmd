@echo off

if %1.==--env. goto SET_ENV
goto HELP

:HELP
echo Usage: khiops-env [-h] [--env]
echo khiops-env is an internal script intended to be used by Khiops tool and Khiops'
echo wrappers only.
echo If the --env flag is used, the environment list is printed on the standard output
echo.
echo The following variables are used to set the path and classpath
echo for the prerequisite of Khiops.
echo.
echo KHIOPS_HOME: home directory of Khiops, on Windows only
echo.
echo KHIOPS_PATH: path of Khiops' executable, to add in path
echo KHIOPS_MPI_COMMAND: MPI command to call the Khiops tool
echo KHIOPS_JAVA_PATH: directory of the jvm.dll, to add in path
echo KHIOPS_CLASSPATH: Khiops java libraries, to add in classpath
echo.
echo If they are not already defined, the following variables used by
echo Khiops are set :
echo.
echo KHIOPS_LAST_RUN_DIR: directory where Khiops writes output command
echo   file and log (when not defined with -e and -o)
echo KHIOPS_PROC_NUMBER: processes number launched by Khiops (it's
echo   default value corresponds to the number of physical cores of
echo   the computer plus one)
echo.
echo The following variables are not defined by default and can be used to
echo change some default properties of Khiops:
echo.
echo KHIOPS_TMP_DIR: Khiops' temporary directory location (default : the
echo   system's default) This location can be modified from the tool as well
echo KHIOPS_MEMORY_LIMIT: Khiops' memory limit in MB (default : the system's memory limit).
echo The minimum value is 100 MB; this setting is ignored if it is above the system's memory limit.
echo It can only be reduced from the tool.
echo KHIOPS_API_MODE: standard or api mode for the management of output result files created by Khiops
echo In standard mode, the result files are stored in the train database directory,
echo   unless an absolute path is specified, and the file extension is forced if necessary.
echo In api mode, the result files are stored in the current working directory, using the specified results as is.
echo . default behavior if not set: standard mode
echo . set to 'true' to force standard mode
echo . set to 'false' to force api mode
echo KHIOPS_RAW_GUI: graphical user interface for file name selection
echo . default behavior if not set: depending on the file drivers available for Khiops
echo . set to 'true' to allow file name selection with uri schemas
echo . set to 'false' to allow local file name selection only with a file selection dialog

if not %2.==. exit /b 1
if %1.==-h. exit /b 0
exit /b 1

REM Set Khiops env variables
:SET_ENV

REM Test if Khiops environment already set up
if "%KHIOPS_HOME%".=="". set KHIOPS_HOME=$_EnvKhiopsHome

REM KHIOPS_PATH
set KHIOPS_PATH=%KHIOPS_HOME%\bin

REM KHIOPS_CLASSPATH
set KHIOPS_CLASSPATH=%KHIOPS_HOME%\bin\norm.jar
set KHIOPS_CLASSPATH=%KHIOPS_HOME%\bin\khiops.jar;%KHIOPS_CLASSPATH%

REM KHIOPS_LAST_RUN_DIR
if "%KHIOPS_LAST_RUN_DIR%".=="". set KHIOPS_LAST_RUN_DIR=%USERPROFILE%\khiops_data\lastrun

REM KHIOPS_PROC_NUMBER
if "%KHIOPS_PROC_NUMBER%".=="". set KHIOPS_PROC_NUMBER=$_EnvProcessNumber

REM KHIOPS_MPI_COMMAND
REM Priority
REM   0: Idle
REM   1: Below normal
REM   2: Normal

set KHIOPS_MPI_COMMAND="%MSMPI_BIN%mpiexec" -al spr:P -n %KHIOPS_PROC_NUMBER% /priority 1
if %KHIOPS_PROC_NUMBER%==1 set KHIOPS_MPI_COMMAND=

REM Set Java environment
set KHIOPS_JAVA_PATH=%KHIOPS_HOME%\jre\bin\server\;%KHIOPS_HOME%\jre\bin\

REM Print the environment list on the standard output
echo KHIOPS_HOME %KHIOPS_HOME%
echo KHIOPS_PATH %KHIOPS_PATH%
echo KHIOPS_MPI_COMMAND %KHIOPS_MPI_COMMAND%
echo KHIOPS_JAVA_PATH %KHIOPS_JAVA_PATH%
echo KHIOPS_CLASSPATH %KHIOPS_CLASSPATH%
echo KHIOPS_LAST_RUN_DIR %KHIOPS_LAST_RUN_DIR%
echo KHIOPS_PROC_NUMBER %KHIOPS_PROC_NUMBER%
echo KHIOPS_TMP_DIR %KHIOPS_TMP_DIR%
echo KHIOPS_MEMORY_LIMIT %KHIOPS_MEMORY_LIMIT%
echo KHIOPS_API_MODE %KHIOPS_API_MODE%
echo KHIOPS_RAW_GUI %KHIOPS_RAW_GUI%
exit /b 0