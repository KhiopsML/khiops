!include "FileFunc.nsh"
!include "x64.nsh"
!include "WordFunc.nsh"

# To deactivate the requirements installation define DO_NOT_INSTALL_REQUIREMENTS

# Detects and loads Java installation path and the JRE version
# - Defines JavaInstalledVersion and JavaInstallationPath
# - If the detection fails JavaInstalledVersion is set to ""
Function DetectAndLoadJavaEnvironment
  # Installed version
  Var /GLOBAL JavaInstalledVersion

  # Installation path
  Var /GLOBAL JavaInstallationPath

  # Set the 64 bit registry to detect the correct version of Java
  SetRegView 64

  # Detection of JRE for Java >= 10
  StrCpy $JavaInstalledVersion ""
  StrCpy $JavaInstallationPath ""
  StrCpy $1 "SOFTWARE\JavaSoft\JRE"
  StrCpy $2 0
  ReadRegStr $2 HKLM "$1" "CurrentVersion"
  ${If} $2 != ""
     ReadRegStr $3 HKLM "$1\$2" "JavaHome"
      ${If} $3 != ""
          StrCpy $JavaInstalledVersion $2
          StrCpy $JavaInstallationPath $3
      ${EndIf}
  ${EndIf}

  # Debug message
  !ifdef DEBUG
    Messagebox MB_OK "JRE (>=10.0): required=${JavaRequiredVersion}, installed=$JavaInstalledVersion, path=$JavaInstallationPath"
  !endif

  # Detection of JRE for Java < 10.0
  ${If} $JavaInstalledVersion == ""
      StrCpy $JavaInstallationPath ""
      StrCpy $1 "SOFTWARE\JavaSoft\Java Runtime Environment"
      StrCpy $2 0
      ReadRegStr $2 HKLM "$1" "CurrentVersion"
      ${If} $2 != ""
          ReadRegStr $3 HKLM "$1\$2" "JavaHome"
          ${If} $3 != ""
              StrCpy $JavaInstallationPath $3
              StrCpy $JavaInstalledVersion $2
          ${EndIf}
      ${EndIf}
  ${EndIf}

  # Debug message
  !ifdef DEBUG
    Messagebox MB_OK "JRE (< 10.0): required=${JavaRequiredVersion}, installed=$JavaInstalledVersion, path=$JavaInstallationPath"
  !endif

  # Get back to 32 bit registry
  SetRegView 32
FunctionEnd

# Installs Java
Function InstallJava
!ifndef DO_NOT_INSTALL_REQUIREMENTS
  # Write the JRE installer
  SetOutPath $TEMP
  File ${JRE_INSTALLER_PATH}

  # Execute the JRE installer silently
  Var /Global JRE_INSTALLER_FILENAME
  ${GetFileName} ${JRE_INSTALLER_PATH} $JRE_INSTALLER_FILENAME
  nsexec::Exec '"$TEMP\$JRE_INSTALLER_FILENAME" INSTALL_SILENT=1 REBOOT=0'
  Pop $0
  DetailPrint "Installation of Java JRE: $0"

  # Delete JRE installer
  Delete "$TEMP\$JRE_INSTALLER_FILENAME"

  # Load the Java Environment
  Call DetectAndLoadJavaEnvironment

  # Fail if the required version is newer than installed version
  ${VersionCompare} "${JavaRequiredVersion}" "$JavaInstalledVersion" $0
  ${If} $0 == 1
      Messagebox MB_OK "Could not install Java runtime (version ${JavaRequiredVersion}): Khiops will run only in batch mode. Try installing Java JRE directly before installing to Khiops." /SD IDOK
  ${EndIf}
!endif
FunctionEnd


# Detects MPI and loads its version
# - Defines MPIInstalledVersion
# - If not installed MPIInstalledVersion is equal to 0
Function DetectAndLoadMPIEnvironment
  # Installed version
  Var /GLOBAL MPIInstalledVersion
	StrCpy $MPIInstalledVersion 0

  # Look in the registry for the MPI installation
  StrCpy $1 "SOFTWARE\Microsoft\MPI"
  StrCpy $2 0
  ReadRegStr $2 HKLM "$1" "Version"
  StrCpy $MPIInstalledVersion $2
FunctionEnd


# Installs MPI
Function InstallMPI
!ifndef DO_NOT_INSTALL_REQUIREMENTS
  # Save MPI installer
  SetOutPath $TEMP
  File ${MSMPI_INSTALLER_PATH}

  # Execute MPI installer
  Var /Global MSMPI_INSTALLER_FILENAME
  ${GetFileName} ${MSMPI_INSTALLER_PATH} $MSMPI_INSTALLER_FILENAME
  nsexec::Exec '"$TEMP\$MSMPI_INSTALLER_FILENAME" -unattend -force -minimal'
  Pop $0
  DetailPrint "Installation of MPI: $0"

  # Delete MSMPI installer
  Delete "$TEMP\$MSMPI_INSTALLER_FILENAME"

  # Load MPI environment (MPIInstalledVersion)
  Call DetectAndLoadMPIEnvironment

  # Show an error if the required version is newer than installed version
  ${VersionCompare} "${MPIRequiredVersion}" "$MPIInstalledVersion" $0
  ${If} $0 == 1
      Messagebox MB_OK "Could not install MPI runtime (version ${MPIRequiredVersion}): Khiops will not run. Try installing Microsoft MPI directly before installing Khiops." /SD IDOK
  ${EndIf}
!endif
FunctionEnd
