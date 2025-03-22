!include "FileFunc.nsh"
!include "WordFunc.nsh"

# To deactivate the requirements installation define DO_NOT_INSTALL_REQUIREMENTS


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
