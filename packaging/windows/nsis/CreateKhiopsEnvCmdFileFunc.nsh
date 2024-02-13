!include "FileFunc.nsh"
!include "x64.nsh"

# Macro to create the khiops_env.cmd script
# Example:
#   ${CreateKhiopsEnvCmdFile} "$INSTDIR\khiops_env.cmd" "$INSTDIR" "4"
#
!define CreateKhiopsEnvCmdFile "!insertmacro CreateKhiopsEnvCmdFile"
!macro CreateKhiopsEnvCmdFile FileName KhiopsHome PhysicalCoresNumber
  Push "${PhysicalCoresNumber}"
  Push "${KhiopsHome}"
  Push "${FileName}"
  Call CreateKhiopsEnvCmdFile
!macroend

# Function to be used with the macro defined above
Function CreateKhiopsEnvCmdFile
    # Define function parameters
    Var /GLOBAL _EnvFileName
    Var /GLOBAL _EnvKhiopsHome
    Var /GLOBAL _EnvProcessNumber

    # Retrieve parameters from stack
    Pop $_EnvFileName
    Pop $_EnvKhiopsHome
    Pop $_EnvProcessNumber

    # Open the file to create
    FileOpen $0 "$_EnvFileName" w

    # Write file contents
    FileWrite $0 `@echo off$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `if %1.==--env. goto SET_ENV$\r$\n`
    FileWrite $0 `goto HELP$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `:HELP$\r$\n`
    FileWrite $0 `echo Usage: khiops-env [-h] [--env]$\r$\n`
    FileWrite $0 `echo khiops-env is an internal script intended to be used by Khiops tool and Khiops'$\r$\n`
    FileWrite $0 `echo wrappers only.$\r$\n`
    FileWrite $0 `echo If the --env flag is used, the environment list is printed on the standard output$\r$\n`
    FileWrite $0 `echo.$\r$\n`
    FileWrite $0 `echo The following variables are used to set the path and classpath$\r$\n`
    FileWrite $0 `echo for the prerequisite of Khiops.$\r$\n`
    FileWrite $0 `echo.$\r$\n`
    FileWrite $0 `echo KHIOPS_HOME: home directory of Khiops, on Windows only$\r$\n`
    FileWrite $0 `echo.$\r$\n`
    FileWrite $0 `echo KHIOPS_PATH: path of Khiops' executable, to add in path$\r$\n`
    FileWrite $0 `echo KHIOPS_MPI_COMMAND: MPI command to call the Khiops tool$\r$\n`
    FileWrite $0 `echo KHIOPS_MPI_LIB: MPI library path used by the Khiops tool$\r$\n`
    FileWrite $0 `echo KHIOPS_JAVA_PATH: path of Java tool, to add in path$\r$\n`
    FileWrite $0 `echo KHIOPS_CLASSPATH: Khiops java libraries, to add in classpath$\r$\n`
    FileWrite $0 `echo.$\r$\n`
    FileWrite $0 `echo If they are not already defined, the following variables used by$\r$\n`
    FileWrite $0 `echo Khiops are set :$\r$\n`
    FileWrite $0 `echo.$\r$\n`
    FileWrite $0 `echo KHIOPS_LAST_RUN_DIR: directory where Khiops writes output command$\r$\n`
    FileWrite $0 `echo   file and log (when not defined with -e and -o)$\r$\n`
    FileWrite $0 `echo KHIOPS_PROC_NUMBER: processes number launched by Khiops (it's$\r$\n`
    FileWrite $0 `echo   default value corresponds to the number of physical cores of$\r$\n`
    FileWrite $0 `echo   the computer plus one)$\r$\n`
    FileWrite $0 `echo.$\r$\n`
    FileWrite $0 `echo The following variables are not defined by default and can be used to$\r$\n`
    FileWrite $0 `echo change some default properties of Khiops:$\r$\n`
    FileWrite $0 `echo.$\r$\n`
    FileWrite $0 `echo KHIOPS_TMP_DIR: Khiops' temporary directory location (default : the$\r$\n`
    FileWrite $0 `echo   system's default) This location can be modified from the tool as well$\r$\n`
    FileWrite $0 `echo KHIOPS_MEMORY_LIMIT: Khiops' memory limit (default : the system's$\r$\n`
    FileWrite $0 `echo   memory limit). This setting is ignored if it is above the system's$\r$\n`
    FileWrite $0 `echo   memory limit. It can only be reduced from the tool$\r$\n`
    FileWrite $0 `echo KHIOPS_API_MODE: standard or api mode for the management of output result files created by Khiops$\r$\n`
    FileWrite $0 `echo                 In standard mode, the result files are stored in the train database directory,$\r$\n`
    FileWrite $0 `echo                   unless an absolute path is specified, and the file extension is forced if necessary.$\r$\n`
    FileWrite $0 `echo                 In api mode, the result files are stored in the current working directory, using the specified results as is.$\r$\n`
    FileWrite $0 `echo                 . default behavior if not set: standard mode$\r$\n`
    FileWrite $0 `echo                 . set to 'true' to force standard mode$\r$\n`
    FileWrite $0 `echo                 . set to 'false' to force api mode$\r$\n`
    FileWrite $0 `echo KHIOPS_RAW_GUI: graphical user interface for file name selection$\r$\n`
    FileWrite $0 `echo                 . default behavior if not set: depending on the file drivers available for Khiops$\r$\n`
    FileWrite $0 `echo                 . set to 'true' to allow file name selection with uri schemas$\r$\n`
    FileWrite $0 `echo                 . set to 'false' to allow local file name selection only with a file selection dialog$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `if not %2.==. exit /b 1$\r$\n`
    FileWrite $0 `if %1.==-h. exit /b 0$\r$\n`
    FileWrite $0 `exit /b 1$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `REM Set Khiops env variables$\r$\n`
    FileWrite $0 `:SET_ENV$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `REM Test if Khiops environment already set up$\r$\n`
    FileWrite $0 `if "%KHIOPS_HOME%".=="". set KHIOPS_HOME=$_EnvKhiopsHome$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `REM KHIOPS_PATH$\r$\n`
    FileWrite $0 `set KHIOPS_PATH=%KHIOPS_HOME%\bin$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `REM KHIOPS_CLASSPATH$\r$\n`
    FileWrite $0 `set KHIOPS_CLASSPATH=%KHIOPS_HOME%\bin\norm.jar$\r$\n`
    FileWrite $0 `set KHIOPS_CLASSPATH=%KHIOPS_HOME%\bin\khiops.jar;%KHIOPS_CLASSPATH%$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `REM KHIOPS_LAST_RUN_DIR$\r$\n`
    FileWrite $0 `if "%KHIOPS_LAST_RUN_DIR%".=="". set KHIOPS_LAST_RUN_DIR=%USERPROFILE%\khiops_data\lastrun$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `REM KHIOPS_PROC_NUMBER$\r$\n`
    FileWrite $0 `if "%KHIOPS_PROC_NUMBER%".=="". set KHIOPS_PROC_NUMBER=$_EnvProcessNumber$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `REM KHIOPS_MPI_COMMAND$\r$\n`
    FileWrite $0 `REM Priority$\r$\n`
    FileWrite $0 `REM   0: Idle$\r$\n`
    FileWrite $0 `REM   1: Below normal$\r$\n`
    FileWrite $0 `REM   2: Normal$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `set KHIOPS_MPI_COMMAND="%MSMPI_BIN%mpiexec" -al spr:P -n %KHIOPS_PROC_NUMBER% /priority 1$\r$\n`
    FileWrite $0 `if %KHIOPS_PROC_NUMBER%==1 set KHIOPS_MPI_COMMAND=$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `REM KHIOPS_JAVA_PATH$\r$\n`
    FileWrite $0 `REM May not work in case of a recent new installation of Java on the PC$\r$\n`
    FileWrite $0 `set KHIOPS_JAVA_PATH=$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `REM Set user Java Home$\r$\n`
    FileWrite $0 `REM Uncomment the following line your own Java version$\r$\n`
    FileWrite $0 `REM set _KHIOPS_JAVA_HOME=C:\Program Files\Java\jre${JavaRequiredFullVersion}$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `:JAVA_HOME_0$\r$\n`
    FileWrite $0 `set _KHIOPS_JAVA_HOME_ORIGIN=java home set by user$\r$\n`
    FileWrite $0 `REM Search if set by user$\r$\n`
    FileWrite $0 `if not "%_KHIOPS_JAVA_HOME%".=="". goto JAVA_HOME_LAST$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `:JAVA_HOME_1$\r$\n`
    FileWrite $0 `set _KHIOPS_JAVA_HOME_ORIGIN=found java recent version in registry$\r$\n`
    FileWrite $0 `REM Search for a recent version of Java (greater than 8)$\r$\n`
    FileWrite $0 `REM Search Java Version from registry and set _KHIOPS_JAVA_VERSION env var$\r$\n`
    FileWrite $0 `for /F "tokens=2*" %%f in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\JRE" -v CurrentVersion 2^>nul') do set _KHIOPS_JAVA_VERSION=%%g$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `REM Search Java Home from registry and set _KHIOPS_JAVA_HOME env var$\r$\n`
    FileWrite $0 `for /F "tokens=2*" %%f in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\JRE\%_KHIOPS_JAVA_VERSION%" -v JavaHome 2^>nul') do set _KHIOPS_JAVA_HOME=%%g$\r$\n`
    FileWrite $0 `if not "%_KHIOPS_JAVA_HOME%".=="". goto JAVA_HOME_LAST$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `:JAVA_HOME_2$\r$\n`
    FileWrite $0 `set _KHIOPS_JAVA_HOME_ORIGIN=found java version in registry$\r$\n`
    FileWrite $0 `REM Search for a previous version of Java$\r$\n`
    FileWrite $0 `REM Search Java Version from registry and set _KHIOPS_JAVA_VERSION env var$\r$\n`
    FileWrite $0 `for /F "tokens=2*" %%f in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Runtime Environment" -v CurrentVersion 2^>nul') do set _KHIOPS_JAVA_VERSION=%%g$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `REM Search Java Home from registry and set _KHIOPS_JAVA_HOME env var$\r$\n`
    FileWrite $0 `for /F "tokens=2*" %%f in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Runtime Environment\%_KHIOPS_JAVA_VERSION%" -v JavaHome 2^>nul') do set _KHIOPS_JAVA_HOME=%%g$\r$\n`
    FileWrite $0 `if not "%_KHIOPS_JAVA_HOME%".=="". goto JAVA_HOME_LAST$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `:JAVA_HOME_3$\r$\n`
    FileWrite $0 `set _KHIOPS_JAVA_HOME_ORIGIN=found in java installation sub dirs$\r$\n`
    FileWrite $0 `REM Heuristic search in sub-directories of java installation dir$\r$\n`
    FileWrite $0 `if not exist "%programFiles%\java" goto JAVA_HOME_LAST$\r$\n`
    # Following instruction surrounding by simple quotes ', to allow internal use of back-quotes `
    FileWrite $0 'FOR /F "usebackq delims=" %%i IN (`where /R "%programFiles%\java" jvm.dll`) DO (set KHIOPS_JAVA_PATH=%%~di%%~pi)$\r$\n'
    FileWrite $0 `if exist "%KHIOPS_JAVA_PATH%\jvm.dll" goto JAVA_END$\r$\n`
    FileWrite $0 `if not exist "%KHIOPS_JAVA_PATH%\jvm.dll" set KHIOPS_JAVA_PATH=$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `:JAVA_HOME_4$\r$\n`
    FileWrite $0 `set _KHIOPS_JAVA_HOME_ORIGIN=default installation settings$\r$\n`
    FileWrite $0 `REM Set default Java Home if not found in registry$\r$\n`
    FileWrite $0 `if "%_KHIOPS_JAVA_HOME%".=="". set _KHIOPS_JAVA_HOME=C:\Program Files\Java\jre${JavaRequiredFullVersion}$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `:JAVA_HOME_LAST$\r$\n`
    FileWrite $0 `REM Add java runtime dir to KHIOPS_JAVA_PATH$\r$\n`
    FileWrite $0 `if exist "%_KHIOPS_JAVA_HOME%\bin\client\jvm.dll" set KHIOPS_JAVA_PATH=%_KHIOPS_JAVA_HOME%\bin\client$\r$\n`
    FileWrite $0 `if exist "%_KHIOPS_JAVA_HOME%\bin\server\jvm.dll" set KHIOPS_JAVA_PATH=%_KHIOPS_JAVA_HOME%\bin\server$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `:JAVA_END$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `$\r$\n`
    FileWrite $0 `REM Print the environment list on the standard output$\r$\n`
    FileWrite $0 `echo KHIOPS_HOME %KHIOPS_HOME%$\r$\n`
    FileWrite $0 `echo KHIOPS_PATH %KHIOPS_PATH%$\r$\n`
    FileWrite $0 `echo KHIOPS_MPI_COMMAND %KHIOPS_MPI_COMMAND%$\r$\n`
    FileWrite $0 `echo KHIOPS_MPI_LIB %KHIOPS_MPI_LIB%$\r$\n`
    FileWrite $0 `echo KHIOPS_JAVA_PATH %KHIOPS_JAVA_PATH%$\r$\n`
    FileWrite $0 `echo KHIOPS_CLASSPATH %KHIOPS_CLASSPATH%$\r$\n`
    FileWrite $0 `echo KHIOPS_LAST_RUN_DIR %KHIOPS_LAST_RUN_DIR%$\r$\n`
    FileWrite $0 `echo KHIOPS_PROC_NUMBER %KHIOPS_PROC_NUMBER%$\r$\n`
    FileWrite $0 `echo KHIOPS_TMP_DIR %KHIOPS_TMP_DIR%$\r$\n`
    FileWrite $0 `echo KHIOPS_MEMORY_LIMIT %KHIOPS_MEMORY_LIMIT%$\r$\n`
    FileWrite $0 `echo KHIOPS_API_MODE %KHIOPS_API_MODE%$\r$\n`
    FileWrite $0 `echo KHIOPS_RAW_GUI %KHIOPS_RAW_GUI%$\r$\n`
    FileWrite $0 `exit /b 0$\r$\n`

    # Close file
    FileClose $0
FunctionEnd
