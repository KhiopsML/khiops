!include "FileFunc.nsh"
!include "x64.nsh"

# Macro to create the khiops.cmd script
# Example:
#   ${CreateKhiopsCmdFile} "$INSTDIR\khiops.cmd" "MODL" "" "$INSTDIR" "scenario._kh" "log.txt" "1"
#
!define CreateKhiopsCmdFile "!insertmacro CreateKhiopsCmdFile"
!macro CreateKhiopsCmdFile FileName ToolName BinSuffix KhiopsHome ScenarioFileName LogFileName ParallelMode
  Push "${ParallelMode}"
  Push "${LogFileName}"
  Push "${ScenarioFileName}"
  Push "${KhiopsHome}"
  Push "${BinSuffix}"
  Push "${ToolName}"
  Push "${FileName}"
  Call CreateKhiopsCmdFile
!macroend


Function CreateKhiopsCmdFile
  # Function parameters
  Var /GLOBAL _FileName
  Var /GLOBAL _ToolName
  Var /GLOBAL _BinSuffix
  Var /GLOBAL _KhiopsHome
  Var /GLOBAL _ScenarioFileName
  Var /GLOBAL _LogFileName
  Var /GLOBAL _ParallelMode

  # Retrieve parameters from stack
  Pop $_FileName
  Pop $_ToolName
  Pop $_BinSuffix
  Pop $_KhiopsHome
  Pop $_ScenarioFileName
  Pop $_LogFileName
  Pop $_ParallelMode

  # Open file to create
  FileOpen $0 "$_FileName" w

  # Write file
  FileWrite $0 `@echo off$\r$\n`
  FileWrite $0 `setlocal$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `REM ========================================================$\r$\n`
  FileWrite $0 `REM See the khiops_env script for full documentation on the$\r$\n`
  FileWrite $0 `REM environment variables used by Khiops$\r$\n`
  FileWrite $0 `REM ========================================================$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `REM ========================================================$\r$\n`
  FileWrite $0 `REM Initialization of the installation directory of Khiops$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `REM Test is Khiops environment already set up$\r$\n`
  FileWrite $0 `if "%KHIOPS_HOME%".=="". set KHIOPS_HOME=$_KhiopsHome$\r$\n`
  FileWrite $0 `if not exist "%KHIOPS_HOME%\bin$_BinSuffix\$_ToolName.exe" goto ERR_PATH$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `REM Test if batch mode from parameters$\r$\n`
  FileWrite $0 `set _KHIOPS_BATCH_MODE=$\r$\n`
  FileWrite $0 `for %%i in (%*) do if %%i.==-b. set _KHIOPS_BATCH_MODE=true$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `REM Initialize Khiops env variables$\r$\n`
  FileWrite $0 `call "%KHIOPS_HOME%\bin\khiops_env" --env > NUL$\r$\n`
  FileWrite $0 `if not %_KHIOPS_BATCH_MODE%.==true. if not exist "%KHIOPS_JAVA_PATH%\jvm.dll" goto ERR_JAVA$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `REM Set path$\r$\n`
  FileWrite $0 `set path=%KHIOPS_PATH%;%KHIOPS_JAVA_PATH%;%path%$\r$\n`
  FileWrite $0 `set classpath=%KHIOPS_CLASSPATH%;%classpath%$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `REM ========================================================$\r$\n`
  FileWrite $0 `REM Start Khiops (with or without parameteres)$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `if %1.==. goto NOPARAMS$\r$\n`
  FileWrite $0 `if not %1.==. goto PARAMS$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `REM Start without parameters$\r$\n`
  FileWrite $0 `:NOPARAMS$\r$\n`
  FileWrite $0 `if not exist "%KHIOPS_LAST_RUN_DIR%" md "%KHIOPS_LAST_RUN_DIR%"$\r$\n`
  FileWrite $0 `if not exist "%KHIOPS_LAST_RUN_DIR%" goto PARAMS$\r$\n`
  ${If} $_ParallelMode == "0"
      FileWrite $0 `"%KHIOPS_PATH%$_BinSuffix\$_ToolName" -o "%KHIOPS_LAST_RUN_DIR%\$_ScenarioFileName" -e "%KHIOPS_LAST_RUN_DIR%\$_LogFileName"$\r$\n`
  ${Else}
      FileWrite $0 `%KHIOPS_MPI_COMMAND% "%KHIOPS_PATH%$_BinSuffix\$_ToolName" -o "%KHIOPS_LAST_RUN_DIR%\$_ScenarioFileName" -e "%KHIOPS_LAST_RUN_DIR%\$_LogFileName"$\r$\n`
  ${EndIf}
  FileWrite $0 `goto END$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `REM Start with parameters$\r$\n`
  FileWrite $0 `:PARAMS$\r$\n`
  ${If} $_ParallelMode == "0"
    FileWrite $0 `"%KHIOPS_PATH%$_BinSuffix\$_ToolName" %*$\r$\n`
  ${Else}
    FileWrite $0 `%KHIOPS_MPI_COMMAND% "%KHIOPS_PATH%$_BinSuffix\$_ToolName" %*$\r$\n`
  ${EndIf}
  FileWrite $0 `goto END$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `REM ========================================================$\r$\n`
  FileWrite $0 `REM Error messages$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `:ERR_PATH$\r$\n`
  FileWrite $0 `start "KHIOPS CONFIG PROBLEM" echo ERROR Incorrect installation directory for Khiops (File $_ToolName.exe not found in directory %KHIOPS_PATH%$_BinSuffix)$\r$\n`
  FileWrite $0 `exit /b 1$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `:ERR_JAVA$\r$\n`
  FileWrite $0 `start "KHIOPS CONFIG PROBLEM" echo ERROR Java not correctly installed, jvm.dll not found under java directory tree %_KHIOPS_JAVA_HOME% (%_KHIOPS_JAVA_HOME_ORIGIN%): see khiops_env.cmd file in %KHIOPS_HOME%\bin, after line 'Set user Java Home'$\r$\n`
  FileWrite $0 `exit /b 1$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `:END$\r$\n`
  FileWrite $0 `endlocal$\r$\n`
  FileWrite $0 `$\r$\n`
  FileWrite $0 `REM Return 1 if fatal error, 2 if error(s), 0 otherwise$\r$\n`
  FileWrite $0 `exit /b %errorlevel%$\r$\n`

  # Close file
  FileClose $0
FunctionEnd
