REM compute the number of processes to use for Khiops.
if "%KHIOPS_PROC_NUMBER%". == "". for /f %%i in ('"%~dp0_khiopsgetprocnumber"') do set "KHIOPS_PROC_NUMBER=%%i"
