# Khiops installer builder NSIS script

# Set Unicode to avoid warning 7998: "ANSI targets are deprecated"
Unicode True

# Set compresion to LZMA (faster)
SetCompressor /SOLID lzma
#SetCompress off

# Include NSIS librairies
!include "LogicLib.nsh"
!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "x64.nsh"
!include "winmessages.nsh"

# Include Custom libraries
!include "KhiopsGlobals.nsh"
!include "KhiopsPrerequisiteFunc.nsh"
!include "ReplaceInFile.nsh"



# Definitions for registry change notification
!define SHCNE_ASSOCCHANGED 0x8000000
!define SHCNF_IDLIST 0

# Get installation folder from registry if available
InstallDirRegKey HKLM Software\khiops ""

# Request admin privileges
RequestExecutionLevel admin

# Make it aware of HiDPI screens
ManifestDPIAware true

# Macro to check input parameter definitions
!macro CheckInputParameter ParameterName
  !ifndef ${ParameterName}
    !error "${ParameterName} is not defined. Use the flag '-D${ParameterName}=...' to define it."
  !endif
!macroend

# Check the mandatory input definitions
!insertmacro CheckInputParameter KHIOPS_VERSION
!insertmacro CheckInputParameter KHIOPS_REDUCED_VERSION
!insertmacro CheckInputParameter KHIOPS_WINDOWS_BUILD_DIR
!insertmacro CheckInputParameter KHIOPS_VIZ_INSTALLER_PATH
!insertmacro CheckInputParameter KHIOPS_COVIZ_INSTALLER_PATH
!insertmacro CheckInputParameter JRE_PATH
!insertmacro CheckInputParameter MSMPI_INSTALLER_PATH
!insertmacro CheckInputParameter MSMPI_VERSION
!insertmacro CheckInputParameter KHIOPS_SAMPLES_DIR
!insertmacro CheckInputParameter KHIOPS_DOC_DIR

# Application name and installer file name
Name "Khiops ${KHIOPS_VERSION}"
OutFile "khiops-${KHIOPS_VERSION}-setup.exe"

########################
# Variable definitions #
########################

# Requirements installation flags
Var /GLOBAL MPIInstallationNeeded

# Requirements installation messages
Var /GLOBAL MPIInstallationMessage

# Previous Uninstaller data
Var /GLOBAL PreviousUninstaller
Var /GLOBAL PreviousVersion

# %Public%, %AllUsersProfile% (%ProgramData%) and samples directory
Var /GLOBAL WinPublicDir
Var /GLOBAL AllUsersProfileDir
Var /GLOBAL GlobalKhiopsDataDir
Var /GLOBAL SamplesInstallDir

# Root key for the uninstaller in the windows registry
!define UninstallerKey "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"

#####################################
# Modern UI Interface Configuration #
#####################################

# General configuration
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP ".\images\headerimage.bmp"
!define MUI_HEADERIMAGE_LEFT
!define MUI_WELCOMEFINISHPAGE_BITMAP ".\images\welcomefinish.bmp"
!define MUI_ABORTWARNING
!define MUI_ICON ".\images\installer.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\win-uninstall.ico"
BrandingText "Orange"

# Welcome page
!define MUI_WELCOMEPAGE_TITLE "Welcome to the Khiops ${KHIOPS_VERSION} Setup Wizard"
!define MUI_WELCOMEPAGE_TEXT \
    "Khiops is a data mining tool includes data preparation and scoring, visualization, coclustering and covisualization.$\r$\n$\r$\n$\r$\n$\r$\n$(MUI_${MUI_PAGE_UNINSTALLER_PREFIX}TEXT_WELCOME_INFO_TEXT)"
!insertmacro MUI_PAGE_WELCOME

# Licence page
!insertmacro MUI_PAGE_LICENSE "..\..\..\LICENSE"

# Custom page for requirements software
Page custom RequirementsPageShow RequirementsPageLeave

# Install directory choice page
!insertmacro MUI_PAGE_DIRECTORY

# Install files choice page
!insertmacro MUI_PAGE_INSTFILES

# Final page
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Create desktop shortcut"
!define MUI_FINISHPAGE_RUN_FUNCTION "CreateDesktopShortcuts"
!define MUI_FINISHPAGE_TEXT "$\r$\n$\r$\nThank you for installing Khiops."
!define MUI_FINISHPAGE_LINK "khiops.org"
!define MUI_FINISHPAGE_LINK_LOCATION "https://khiops.org"
!insertmacro MUI_PAGE_FINISH

# Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

# Language (must be defined after uninstaller)
!insertmacro MUI_LANGUAGE "English"

#######################
# Version Information #
#######################

VIProductVersion "${KHIOPS_REDUCED_VERSION}.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "Khiops"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "Orange"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Copyright (c) 2023-2025 Orange. All rights reserved."
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "Khiops Installer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${KHIOPS_VERSION}"

######################
# Installer Sections #
######################

Section "Install" SecInstall
  # In order to have shortcuts and documents for all users
  SetShellVarContext all
  
  # Detect Java
  Call RequirementsDetection


  # MPI installation is always required, because Khiops is linked with MPI DLL
  ${If} $MPIInstallationNeeded == "1"
      Call InstallMPI
  ${EndIf}

  # Activate file overwrite
  SetOverwrite on

  # Install executables and java libraries
  SetOutPath "$INSTDIR\bin"
  File "${KHIOPS_WINDOWS_BUILD_DIR}\bin\MODL.exe"
  File "${KHIOPS_WINDOWS_BUILD_DIR}\bin\MODL_Coclustering.exe"
  File "${KHIOPS_WINDOWS_BUILD_DIR}\bin\_khiopsgetprocnumber.exe"
  File "${KHIOPS_WINDOWS_BUILD_DIR}\jars\norm.jar"
  File "${KHIOPS_WINDOWS_BUILD_DIR}\jars\khiops.jar"
  File "${KHIOPS_WINDOWS_BUILD_DIR}\tmp\khiops_env.cmd"
  File "..\khiops.cmd"
  File "..\khiops_coclustering.cmd"

  # Install Docs
  SetOutPath "$INSTDIR"
  File "/oname=LICENSE.txt" "..\..\..\LICENSE"
  File "..\..\common\khiops\README.txt"
  File "..\..\common\khiops\WHATSNEW.txt"
  SetOutPath "$INSTDIR\doc"
  File /nonfatal /a /r "${KHIOPS_DOC_DIR}\"

  # Install icons
  SetOutPath "$INSTDIR\bin\icons"
  File ".\images\installer.ico"
  File "..\..\common\images\khiops.ico"
  File "..\..\common\images\khiops_coclustering.ico"

  # Set the samples directory to be located either within %PUBLIC% or %ALLUSERSPROFILE% as fallback
  ReadEnvStr $WinPublicDir PUBLIC
  ReadEnvStr $AllUsersProfileDir ALLUSERSPROFILE
  ${If} $WinPublicDir != ""
    StrCpy $GlobalKhiopsDataDir "$WinPublicDir\khiops_data"
  ${ElseIf} $AllUsersProfileDir != ""
    StrCpy $GlobalKhiopsDataDir "$AllUsersProfileDir\khiops_data"
  ${Else}
    StrCpy $GlobalKhiopsDataDir ""
  ${EndIf}

  # Debug message
  !ifdef DEBUG
    ${If} $GlobalKhiopsDataDir == ""
      Messagebox MB_OK "Could find PUBLIC nor ALLUSERSPROFILE directories. Samples not installed."
    ${Else}
      Messagebox MB_OK "Samples will be installed at $GlobalKhiopsDataDir\samples."
    ${EndIf}
  !endif

  # Install samples only if the directory is defined
  ${If} $GlobalKhiopsDataDir != ""
    StrCpy $SamplesInstallDir "$GlobalKhiopsDataDir\samples"
    SetOutPath "$SamplesInstallDir"
    File "/oname=README.txt" "${KHIOPS_SAMPLES_DIR}\README.md"
    SetOutPath "$SamplesInstallDir\Adult"
    File "${KHIOPS_SAMPLES_DIR}\Adult\Adult.kdic"
    File "${KHIOPS_SAMPLES_DIR}\Adult\Adult.txt"
    SetOutPath "$SamplesInstallDir\Iris"
    File "${KHIOPS_SAMPLES_DIR}\Iris\Iris.kdic"
    File "${KHIOPS_SAMPLES_DIR}\Iris\Iris.txt"
    SetOutPath "$SamplesInstallDir\Mushroom"
    File "${KHIOPS_SAMPLES_DIR}\Mushroom\Mushroom.kdic"
    File "${KHIOPS_SAMPLES_DIR}\Mushroom\Mushroom.txt"
    SetOutPath "$SamplesInstallDir\Letter"
    File "${KHIOPS_SAMPLES_DIR}\Letter\Letter.kdic"
    File "${KHIOPS_SAMPLES_DIR}\Letter\Letter.txt"
    SetOutPath "$SamplesInstallDir\SpliceJunction"
    File "${KHIOPS_SAMPLES_DIR}\SpliceJunction\SpliceJunction.kdic"
    File "${KHIOPS_SAMPLES_DIR}\SpliceJunction\SpliceJunction.txt"
    File "${KHIOPS_SAMPLES_DIR}\SpliceJunction\SpliceJunctionDNA.txt"
    SetOutPath "$SamplesInstallDir\Accidents"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\Accidents.kdic"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\Accidents.txt"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\Places.txt"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\Users.txt"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\Vehicles.txt"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\train.py"
    File "/oname=README.txt" "${KHIOPS_SAMPLES_DIR}\Accidents\README.md"
    SetOutPath "$SamplesInstallDir\Accidents\raw"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\raw\AccidentsPreprocess.kdic"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\raw\Description_BD_ONISR.pdf"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\raw\Licence_Ouverte.pdf"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\raw\caracteristiques-2018.csv"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\raw\lieux-2018.csv"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\raw\usagers-2018.csv"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\raw\vehicules-2018.csv"
    File "${KHIOPS_SAMPLES_DIR}\Accidents\raw\preprocess.py"
    File "/oname=README.txt" "${KHIOPS_SAMPLES_DIR}\Accidents\raw\README.md"
    SetOutPath "$SamplesInstallDir\AccidentsSummary"
    File "${KHIOPS_SAMPLES_DIR}\AccidentsSummary\Accidents.kdic"
    File "${KHIOPS_SAMPLES_DIR}\AccidentsSummary\Accidents.txt"
    File "${KHIOPS_SAMPLES_DIR}\AccidentsSummary\Vehicles.txt"
    File "/oname=README.txt" "${KHIOPS_SAMPLES_DIR}\AccidentsSummary\README.md"
    SetOutPath "$SamplesInstallDir\Customer"
    File "${KHIOPS_SAMPLES_DIR}\Customer\Customer.kdic"
    File "${KHIOPS_SAMPLES_DIR}\Customer\CustomerRecoded.kdic"
    File "${KHIOPS_SAMPLES_DIR}\Customer\Customer.txt"
    File "${KHIOPS_SAMPLES_DIR}\Customer\Address.txt"
    File "${KHIOPS_SAMPLES_DIR}\Customer\Service.txt"
    File "${KHIOPS_SAMPLES_DIR}\Customer\Usage.txt"
    File "${KHIOPS_SAMPLES_DIR}\Customer\sort_and_recode_customer.py"
    File "/oname=README.txt" "${KHIOPS_SAMPLES_DIR}\Customer\README.md"
    SetOutPath "$SamplesInstallDir\Customer\unsorted"
    File "${KHIOPS_SAMPLES_DIR}\Customer\unsorted\Customer-unsorted.txt"
    File "${KHIOPS_SAMPLES_DIR}\Customer\unsorted\Address-unsorted.txt"
    File "${KHIOPS_SAMPLES_DIR}\Customer\unsorted\Service-unsorted.txt"
    File "${KHIOPS_SAMPLES_DIR}\Customer\unsorted\Usage-unsorted.txt"
    SetOutPath "$SamplesInstallDir\CustomerExtended"
    File "${KHIOPS_SAMPLES_DIR}\CustomerExtended\Customer.kdic"
    File "${KHIOPS_SAMPLES_DIR}\CustomerExtended\CustomerRecoded.kdic"
    File "${KHIOPS_SAMPLES_DIR}\CustomerExtended\Customer.txt"
    File "${KHIOPS_SAMPLES_DIR}\CustomerExtended\Address.txt"
    File "${KHIOPS_SAMPLES_DIR}\CustomerExtended\Service.txt"
    File "${KHIOPS_SAMPLES_DIR}\CustomerExtended\Usage.txt"
    File "${KHIOPS_SAMPLES_DIR}\CustomerExtended\City.txt"
    File "${KHIOPS_SAMPLES_DIR}\CustomerExtended\Country.txt"
    File "${KHIOPS_SAMPLES_DIR}\CustomerExtended\Product.txt"
    File "${KHIOPS_SAMPLES_DIR}\CustomerExtended\recode_customer.py"
    File "/oname=README.txt" "${KHIOPS_SAMPLES_DIR}\CustomerExtended\README.md"
  ${EndIf}

  # Install JRE
  SetOutPath $INSTDIR\jre
  File /nonfatal /a /r "${JRE_PATH}\"

  # Install Khiops Visualization App

  # Add the installer file
  SetOutPath $TEMP
  File ${KHIOPS_VIZ_INSTALLER_PATH}

  # Execute Khiops visualization installer:
  # - It is not executed with silent mode so the user can customize the install
  # - It is executed with "cmd /C" so it opens the installer options window
  Var /Global KHIOPS_VIZ_INSTALLER_FILENAME
  ${GetFileName} ${KHIOPS_VIZ_INSTALLER_PATH} $KHIOPS_VIZ_INSTALLER_FILENAME
  ${If} ${Silent}
    nsexec::Exec 'cmd /C "$KHIOPS_VIZ_INSTALLER_FILENAME /S"'
  ${Else}
    nsexec::Exec 'cmd /C "$KHIOPS_VIZ_INSTALLER_FILENAME"'
  ${EndIf}
  Pop $0
  DetailPrint "Installation of Khiops visualization: $0"

  # Delete the installer
  Delete "$TEMP\KHIOPS_VIZ_INSTALLER_FILENAME"


  # Execute Khiops covisualization installer:
  # Same rules as above with the visualization

  # Files to install in installer directory
  File ${KHIOPS_COVIZ_INSTALLER_PATH}

  Var /Global KHIOPS_COVIZ_INSTALLER_FILENAME
  ${GetFileName} ${KHIOPS_COVIZ_INSTALLER_PATH} $KHIOPS_COVIZ_INSTALLER_FILENAME
  ${If} ${Silent}
    nsexec::Exec 'cmd /C "$TEMP\$KHIOPS_COVIZ_INSTALLER_FILENAME /S"'
  ${Else}
    nsexec::Exec 'cmd /C "$TEMP\$KHIOPS_COVIZ_INSTALLER_FILENAME"'
  ${EndIf}
  Pop $0
  DetailPrint "Installation of Khiops covisualization: $0"

  # Delete the installer
  Delete "$TEMP\$KHIOPS_COVIZ_INSTALLER_FILENAME"


  #############################
  # Finalize the installation #
  #############################

  # Setting up the GUI in khiops_env.cmd: replace @GUI_STATUS@ by "true" in the installed file
  Push @GUI_STATUS@ 
  Push 'true' 
  Push all 
  Push all 
  Push $INSTDIR\bin\khiops_env.cmd
  Call ReplaceInFile

  # Setting up MPI in khiops_env.cmd: replace @SET_MPI@ by "SET_MPI_SYSTEM_WIDE" in the installed file
  Push @SET_MPI@
  Push SET_MPI_SYSTEM_WIDE 
  Push all 
  Push all 
  Push $INSTDIR\bin\khiops_env.cmd
  Call ReplaceInFile

  # Setting up IS_CONDA_VAR variable in khiops_env.cmd: replace @SET_MPI@ by an empty string: this is not an installer for conda
  Push @IS_CONDA_VAR@
  Push "" 
  Push all 
  Push all 
  Push $INSTDIR\bin\khiops_env.cmd
  Call ReplaceInFile

  # Create the Khiops shell
  FileOpen $0 "$INSTDIR\bin\shell_khiops.cmd" w
  FileWrite $0 '@echo off$\r$\n'
  FileWrite $0 'REM Open a shell session with access to Khiops$\r$\n'
  FileWrite $0 `if "%KHIOPS_HOME%".=="". set KHIOPS_HOME=$INSTDIR$\r$\n`
  FileWrite $0 'set path=%KHIOPS_HOME%\bin;%path%$\r$\n'
  FileWrite $0 'title Shell Khiops$\r$\n'
  FileWrite $0 '%comspec% /K "echo Welcome to Khiops scripting mode & echo Type khiops -h or khiops_coclustering -h to get help'
  FileClose $0

  # Create the uninstaller
  WriteUninstaller "$INSTDIR\uninstall-khiops.exe"


  #####################################
  # Windows environment customization #
  # ###################################

  # Write registry keys to add Khiops in the Add/Remove Programs pane
  WriteRegStr HKLM "Software\Khiops" "" $INSTDIR
  WriteRegStr HKLM "${UninstallerKey}\Khiops" "UninstallString" '"$INSTDIR\uninstall-khiops.exe"'
  WriteRegStr HKLM "${UninstallerKey}\Khiops" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UninstallerKey}\Khiops" "DisplayName" "Khiops"
  WriteRegStr HKLM "${UninstallerKey}\Khiops" "Publisher" "Orange"
  WriteRegStr HKLM "${UninstallerKey}\Khiops" "DisplayIcon" "$INSTDIR\bin\icons\installer.ico"
  WriteRegStr HKLM "${UninstallerKey}\Khiops" "DisplayVersion" "${KHIOPS_VERSION}"
  WriteRegStr HKLM "${UninstallerKey}\Khiops" "URLInfoAbout" "http://khiops.org"
  WriteRegDWORD HKLM "${UninstallerKey}\Khiops" "NoModify" "1"
  WriteRegDWORD HKLM "${UninstallerKey}\Khiops" "NoRepair" "1"

  # Set as the startup dir for all executable shortcuts (yes it is done with SetOutPath!)
  ${If} $GlobalKhiopsDataDir != ""
    SetOutPath $GlobalKhiopsDataDir
  ${Else}
    SetOutPath $INSTDIR
  ${EndIf}

  # Create application shortcuts in the installation directory
  DetailPrint "Installing Start menu Shortcut..."
  CreateShortCut "$INSTDIR\Khiops.lnk" "$INSTDIR\bin\khiops.cmd" "" "$INSTDIR\bin\icons\khiops.ico" 0 SW_SHOWMINIMIZED
  CreateShortCut "$INSTDIR\Khiops Coclustering.lnk" "$INSTDIR\bin\khiops_coclustering.cmd" "" "$INSTDIR\bin\icons\khiops_coclustering.ico" 0 SW_SHOWMINIMIZED
  ExpandEnvStrings $R0 "%COMSPEC%"
  CreateShortCut "$INSTDIR\Shell Khiops.lnk" "$INSTDIR\bin\shell_khiops.cmd" "" "$R0"

  # Create start menu shortcuts for the executables and documentation
  DetailPrint "Installing Start menu Shortcut..."
  CreateDirectory "$SMPROGRAMS\Khiops"
  CreateShortCut "$SMPROGRAMS\Khiops\Khiops.lnk" "$INSTDIR\bin\khiops.cmd" "" "$INSTDIR\bin\icons\khiops.ico" 0 SW_SHOWMINIMIZED
  CreateShortCut "$SMPROGRAMS\Khiops\Khiops Coclustering.lnk" "$INSTDIR\bin\khiops_coclustering.cmd" "" "$INSTDIR\bin\icons\khiops_coclustering.ico" 0 SW_SHOWMINIMIZED
  ExpandEnvStrings $R0 "%COMSPEC%"
  CreateShortCut "$SMPROGRAMS\Khiops\Shell Khiops.lnk" "$INSTDIR\bin\shell_khiops.cmd" "" "$R0"
  CreateShortCut "$SMPROGRAMS\Khiops\Uninstall.lnk" "$INSTDIR\uninstall-khiops.exe"
  CreateDirectory "$SMPROGRAMS\Khiops\doc"
  CreateShortCut "$SMPROGRAMS\Khiops\doc\Tutorial.lnk" "$INSTDIR\doc\KhiopsTutorial.pdf"
  CreateShortCut "$SMPROGRAMS\Khiops\doc\Khiops.lnk" "$INSTDIR\doc\KhiopsGuide.pdf"
  CreateShortCut "$SMPROGRAMS\Khiops\doc\Khiops Coclustering.lnk" "$INSTDIR\doc\KhiopsCoclusteringGuide.pdf"
  CreateShortCut "$SMPROGRAMS\Khiops\doc\Khiops Visualization.lnk" "$INSTDIR\doc\KhiopsVisualizationGuide.pdf"
  CreateShortCut "$SMPROGRAMS\Khiops\doc\Khiops Covisualization.lnk" "$INSTDIR\doc\KhiopsCovisualizationGuide.pdf"
  SetOutPath "$INSTDIR"

  # Define aliases for the following registry keys (also used in the uninstaller section)
  # - HKLM (all users)
  # - HKCU (current user)
  !define env_hklm 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
  !define env_hkcu 'HKCU "Environment"'

  # Set KHIOPS_HOME for the local machine and current user
  WriteRegExpandStr ${env_hklm} "KHIOPS_HOME" "$INSTDIR"
  WriteRegExpandStr ${env_hkcu} "KHIOPS_HOME" "$INSTDIR"

  # Make sure windows knows about the change
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000

  # Register file association for Khiops visualisation tools #
  # inspired from examples\makensis.nsi

  # Khiops dictionary file extension
  ReadRegStr $R0 HKCR ".kdic" ""
  ${if} $R0 == "Khiops.Dictionary.File"
    DeleteRegKey HKCR "Khiops.Dictionary.File"
  ${EndIf}
  WriteRegStr HKCR ".kdic" "" "Khiops.Dictionary.File"
  WriteRegStr HKCR "Khiops.Dictionary.File" "" "Khiops Dictionary File"
  ReadRegStr $R0 HKCR "Khiops.Dictionary.File\shell\open\command" ""
  ${If} $R0 == ""
    WriteRegStr HKCR "Khiops.Dictionary.File\shell" "" "open"
    WriteRegStr HKCR "Khiops.Dictionary.File\shell\open\command" "" 'notepad.exe "%1"'
  ${EndIf}

  # Khiops scenario file
  ReadRegStr $R0 HKCR "._kh" ""
  ${if} $R0 == "Khiops.File"
    DeleteRegKey HKCR "Khiops.File"
  ${EndIf}
  WriteRegStr HKCR "._kh" "" "Khiops.File"
  WriteRegStr HKCR "Khiops.File" "" "Khiops File"
  WriteRegStr HKCR "Khiops.File\DefaultIcon" "" "$INSTDIR\bin\icons\khiops.ico"
  ReadRegStr $R0 HKCR "Khiops.File\shell\open\command" ""
  ${If} $R0 == ""
    WriteRegStr HKCR "Khiops.File\shell" "" "open"
    WriteRegStr HKCR "Khiops.File\shell\open\command" "" 'notepad.exe "%1"'
  ${EndIf}
  WriteRegStr HKCR "Khiops.File\shell\compile" "" "Execute Khiops Script"
  WriteRegStr HKCR "Khiops.File\shell\compile\command" "" '"$INSTDIR\bin\khiops.cmd" -i "%1"'

  # Khiops coclustering scenario file
  ReadRegStr $R0 HKCR "._khc" ""
  ${if} $R0 == "Khiops.Coclustering.File"
    DeleteRegKey HKCR "Khiops.Coclustering.File"
  ${EndIf}
  WriteRegStr HKCR "._khc" "" "Khiops.Coclustering.File"
  WriteRegStr HKCR "Khiops.Coclustering.File" "" "Khiops Coclustering File"
  WriteRegStr HKCR "Khiops.Coclustering.File\DefaultIcon" "" "$INSTDIR\bin\icons\khiops_coclustering.ico"
  ReadRegStr $R0 HKCR "Khiops.Coclustering.File\shell\open\command" ""
  ${If} $R0 == ""
    WriteRegStr HKCR "Khiops.Coclustering.File\shell" "" "open"
    WriteRegStr HKCR "Khiops.Coclustering.File\shell\open\command" "" 'notepad.exe "%1"'
  ${EndIf}
  WriteRegStr HKCR "Khiops.Coclustering.File\shell\compile" "" "Execute Khiops Coclustering Script"
  WriteRegStr HKCR "Khiops.Coclustering.File\shell\compile\command" "" '"$INSTDIR\bin\khiops_coclustering.cmd" -i "%1"'

  # Notify the file extension changes
  System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, i 0, i 0)'

  # Debug message
  !ifdef DEBUG
    Messagebox MB_OK "Installation finished!"
  !endif

SectionEnd


###############
# Uninstaller #
###############

Section "Uninstall"
  # In order to have shortcuts and documents for all users
  SetShellVarContext all

  # Restore Registry #
  # Unregister file associations
  DetailPrint "Uninstalling Khiops Shell Extensions..."

  # Unregister Khiops dictionary file extension
  ${If} $R0 == "Khiops.Dictionary.File"
    DeleteRegKey HKCR ".kdic"
  ${EndIf}
  DeleteRegKey HKCR "Khiops.Dictionary.File"

  # Unregister Khiops file extension
  ${If} $R0 == "Khiops.File"
    DeleteRegKey HKCR "._kh"
  ${EndIf}
  DeleteRegKey HKCR "Khiops.File"

  # Unregister Khiops coclustering file extension
  ${If} $R0 == "Khiops.Coclustering.File"
    DeleteRegKey HKCR "._khc"
  ${EndIf}
  DeleteRegKey HKCR "Khiops.Coclustering.File"

  # Notify file extension changes
  System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, i 0, i 0)'

  # Delete installation folder key
  DeleteRegKey HKLM "${UninstallerKey}\Khiops"
  DeleteRegKey HKLM "Software\Khiops"

  # Delete environement variable KHIOPS_HOME
  DeleteRegValue ${env_hklm} "KHIOPS_HOME"
  DeleteRegValue ${env_hkcu} "KHIOPS_HOME"

  # Delete deprecated environment variable KhiopsHome
  DeleteRegValue ${env_hklm} "KhiopsHome"
  DeleteRegValue ${env_hkcu} "KhiopsHome"

  # Make sure windows knows about the changes in the environment
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000

  # Delete files #
  # Note: Some directories are removed only if they are completely empty (no "/r" RMDir flag)
  DetailPrint "Deleting Files ..."

  # Delete docs
  Delete "$INSTDIR\LICENSE.txt"
  Delete "$INSTDIR\README.txt"
  Delete "$INSTDIR\WHATSNEW.txt"
  RMDir /r "$INSTDIR\doc"

  # Delete jre
  RMDir /r "$INSTDIR\jre"

  # Delete icons
  RMDir /r "$INSTDIR\bin\icons"

  # Delete executables and scripts
  Delete "$INSTDIR\bin\khiops_env.cmd"
  Delete "$INSTDIR\bin\khiops.cmd"
  Delete "$INSTDIR\bin\khiops_coclustering.cmd"
  Delete "$INSTDIR\bin\MODL.exe"
  Delete "$INSTDIR\bin\MODL_Coclustering.exe"
  Delete "$INSTDIR\bin\_khiopsgetprocnumber.exe"
  Delete "$INSTDIR\bin\norm.jar"
  Delete "$INSTDIR\bin\khiops.jar"
  Delete "$INSTDIR\bin\shell_khiops.cmd"
  RMDir "$INSTDIR\bin"

  # Delete shortcuts from install dir
  Delete "$INSTDIR\Khiops.lnk"
  Delete "$INSTDIR\Khiops Coclustering.lnk"
  Delete "$INSTDIR\Shell Khiops.lnk"

  # Delete the installer
  Delete "$INSTDIR\uninstall-khiops.exe"

  # Remove install directory
  RMDir "$INSTDIR"

  # Delete desktop shortcuts
  Delete "$DESKTOP\Khiops.lnk"
  Delete "$DESKTOP\Khiops Coclustering.lnk"
  Delete "$DESKTOP\Shell Khiops.lnk"

  # Delete Start Menu Shortcuts
  RMDir /r "$SMPROGRAMS\Khiops"

  # Set the samples directory to be located either within %PUBLIC% or %ALLUSERSPROFILE% as fallback
  ReadEnvStr $WinPublicDir PUBLIC
  ReadEnvStr $AllUsersProfileDir ALLUSERSPROFILE
  ${If} $WinPublicDir != ""
    StrCpy $GlobalKhiopsDataDir "$WinPublicDir\khiops_data"
  ${ElseIf} $AllUsersProfileDir != ""
    StrCpy $GlobalKhiopsDataDir "$AllUsersProfileDir\khiops_data"
  ${Else}
    StrCpy $GlobalKhiopsDataDir ""
  ${EndIf}

  # Delete sample datasets
  # We do not remove the whole directory to save the users results from Khiops' analyses
  ${If} $GlobalKhiopsDataDir != ""
    StrCpy $SamplesInstallDir "$GlobalKhiopsDataDir\samples"
    Delete "$SamplesInstallDir\AccidentsSummary\Accidents.kdic"
    Delete "$SamplesInstallDir\AccidentsSummary\Accidents.txt"
    Delete "$SamplesInstallDir\AccidentsSummary\README.txt"
    Delete "$SamplesInstallDir\AccidentsSummary\Vehicles.txt"
    Delete "$SamplesInstallDir\Accidents\Accidents.kdic"
    Delete "$SamplesInstallDir\Accidents\Accidents.txt"
    Delete "$SamplesInstallDir\Accidents\Places.txt"
    Delete "$SamplesInstallDir\Accidents\README.txt"
    Delete "$SamplesInstallDir\Accidents\Users.txt"
    Delete "$SamplesInstallDir\Accidents\Vehicles.txt"
    Delete "$SamplesInstallDir\Accidents\raw\AccidentsPreprocess.kdic"
    Delete "$SamplesInstallDir\Accidents\raw\Description_BD_ONISR.pdf"
    Delete "$SamplesInstallDir\Accidents\raw\Licence_Ouverte.pdf"
    Delete "$SamplesInstallDir\Accidents\raw\README.txt"
    Delete "$SamplesInstallDir\Accidents\raw\caracteristiques-2018.csv"
    Delete "$SamplesInstallDir\Accidents\raw\lieux-2018.csv"
    Delete "$SamplesInstallDir\Accidents\raw\preprocess.py"
    Delete "$SamplesInstallDir\Accidents\raw\usagers-2018.csv"
    Delete "$SamplesInstallDir\Accidents\raw\vehicules-2018.csv"
    Delete "$SamplesInstallDir\Accidents\train.py"
    Delete "$SamplesInstallDir\Adult\Adult.kdic"
    Delete "$SamplesInstallDir\Adult\Adult.txt"
    Delete "$SamplesInstallDir\CustomerExtended\Address.txt"
    Delete "$SamplesInstallDir\CustomerExtended\City.txt"
    Delete "$SamplesInstallDir\CustomerExtended\Country.txt"
    Delete "$SamplesInstallDir\CustomerExtended\Customer.kdic"
    Delete "$SamplesInstallDir\CustomerExtended\Customer.txt"
    Delete "$SamplesInstallDir\CustomerExtended\CustomerRecoded.kdic"
    Delete "$SamplesInstallDir\CustomerExtended\Product.txt"
    Delete "$SamplesInstallDir\CustomerExtended\README.txt"
    Delete "$SamplesInstallDir\CustomerExtended\Service.txt"
    Delete "$SamplesInstallDir\CustomerExtended\Usage.txt"
    Delete "$SamplesInstallDir\CustomerExtended\recode_customer.py"
    Delete "$SamplesInstallDir\Customer\Address.txt"
    Delete "$SamplesInstallDir\Customer\Customer.kdic"
    Delete "$SamplesInstallDir\Customer\Customer.txt"
    Delete "$SamplesInstallDir\Customer\CustomerRecoded.kdic"
    Delete "$SamplesInstallDir\Customer\README.txt"
    Delete "$SamplesInstallDir\Customer\Service.txt"
    Delete "$SamplesInstallDir\Customer\Usage.txt"
    Delete "$SamplesInstallDir\Customer\sort_and_recode_customer.py"
    Delete "$SamplesInstallDir\Customer\unsorted\Address-unsorted.txt"
    Delete "$SamplesInstallDir\Customer\unsorted\Customer-unsorted.txt"
    Delete "$SamplesInstallDir\Customer\unsorted\Service-unsorted.txt"
    Delete "$SamplesInstallDir\Customer\unsorted\Usage-unsorted.txt"
    Delete "$SamplesInstallDir\Iris\Iris.kdic"
    Delete "$SamplesInstallDir\Iris\Iris.txt"
    Delete "$SamplesInstallDir\Letter\Letter.kdic"
    Delete "$SamplesInstallDir\Letter\Letter.txt"
    Delete "$SamplesInstallDir\Mushroom\Mushroom.kdic"
    Delete "$SamplesInstallDir\Mushroom\Mushroom.txt"
    Delete "$SamplesInstallDir\README.txt"
    Delete "$SamplesInstallDir\SpliceJunction\SpliceJunction.kdic"
    Delete "$SamplesInstallDir\SpliceJunction\SpliceJunction.txt"
    Delete "$SamplesInstallDir\SpliceJunction\SpliceJunctionDNA.txt"
    RMDir "$SamplesInstallDir\AccidentsSummary\"
    RMDir "$SamplesInstallDir\Accidents\raw\"
    RMDir "$SamplesInstallDir\Accidents\"
    RMDir "$SamplesInstallDir\Adult\"
    RMDir "$SamplesInstallDir\CustomerExtended\"
    RMDir "$SamplesInstallDir\Customer\unsorted\"
    RMDir "$SamplesInstallDir\Customer\"
    RMDir "$SamplesInstallDir\Iris\"
    RMDir "$SamplesInstallDir\Letter\"
    RMDir "$SamplesInstallDir\Mushroom\"
    RMDir "$SamplesInstallDir\SpliceJunction\"
    RMDir "$SamplesInstallDir"
  ${EndIf}
SectionEnd


#######################
# Installer Functions #
#######################

Function "CreateDesktopShortcuts"
  # Set as the startup dir for all executable shortcuts (yes it is done with SetOutPath!)
  ${If} $GlobalKhiopsDataDir != ""
    SetOutPath $GlobalKhiopsDataDir
  ${Else}
    SetOutPath $INSTDIR
  ${EndIf}

  # Create the shortcuts
  DetailPrint "Installing Desktop Shortcut..."
  CreateShortCut "$DESKTOP\Khiops.lnk" "$INSTDIR\bin\khiops.cmd" "" "$INSTDIR\bin\icons\khiops.ico" 0 SW_SHOWMINIMIZED
  CreateShortCut "$DESKTOP\Khiops Coclustering.lnk" "$INSTDIR\bin\khiops_coclustering.cmd" "" "$INSTDIR\bin\icons\khiops_coclustering.ico" 0 SW_SHOWMINIMIZED
FunctionEnd

# Predefined initialization install function
Function .onInit

  # Read location of the uninstaller
  ReadRegStr $PreviousUninstaller HKLM "${UninstallerKey}\Khiops" "UninstallString"
  ReadRegStr $PreviousVersion HKLM "${UninstallerKey}\Khiops" "DisplayVersion"

  # Ask the user to proceed if there was already a previous Khiops version installed
  # In silent mode: remove previous version
  ${If} $PreviousUninstaller != ""
    MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
       "Khiops $PreviousVersion is already installed. $\n$\nClick OK to remove the \
       previous version $\n$\nor Cancel to cancel this upgrade." \
       /SD IDOK IDOK uninst
    Abort

    # Run the uninstaller
    uninst:
    ClearErrors
    ExecWait '$PreviousUninstaller /S _?=$INSTDIR'

    # Run again the uninstaller to delete the uninstaller itself and the root dir (without waiting)
    # Must not be used in silent mode (may delete files from silent following installation)
    ${IfNot} ${Silent}
       ExecWait '$PreviousUninstaller /S'
    ${EndIf}
  ${EndIf}

  # Choice of default installation directory, for windows 32 or 64
  ${If} $INSTDIR == ""
    ${If} ${RunningX64}
      StrCpy $INSTDIR "$PROGRAMFILES64\khiops"
      # No 32-bit install
    ${EndIf}
  ${EndIf}
FunctionEnd


# Function to show the page for requirements
Function RequirementsPageShow
  # Detect requirements
  Call RequirementsDetection

  # Creation of page, with title and subtitle
  nsDialogs::Create 1018
  !insertmacro MUI_HEADER_TEXT "Check software requirements" "Check Microsoft MPI"

  # Message to show for the Microsoft MPI installation
  ${NSD_CreateLabel} 0 20u 100% 10u $MPIInstallationMessage

  # Show page
  nsDialogs::Show
FunctionEnd


# Requirements detection
# - Detects if the system architecture is 64-bit
# - Detects whether Java JRE and MPI are installed and their versions
Function RequirementsDetection
  # Abort installation if the machine does not have 64-bit architecture
  ${IfNot} ${RunningX64}
    Messagebox MB_OK "Khiops works only on Windows 64 bits: installation will be terminated." /SD IDOK
    Quit
  ${EndIf}

  # Decide if MPI is required by detecting the number of cores
  # Note: This call defines MPIInstalledVersion
  Call DetectAndLoadMPIEnvironment

  # Try to install MPI
  StrCpy $MPIInstallationNeeded "0"
  StrCpy $MPIInstallationMessage ""
 
  # If it is not installed install it
  ${If} $MPIInstalledVersion == "0"
      StrCpy $MPIInstallationMessage "Microsoft MPI version ${MSMPI_VERSION} will be installed"
      StrCpy $MPIInstallationNeeded "1"
  # Otherwise install only if the required version is newer than the installed one
  ${Else}
      ${VersionCompare} "${MPIRequiredVersion}" "$MPIInstalledVersion" $0
      ${If} $0 == 1
        StrCpy $MPIInstallationMessage "Microsoft MPI will be upgraded to version ${MSMPI_VERSION}"
        StrCpy $MPIInstallationNeeded "1"
      ${Else}
        StrCpy $MPIInstallationMessage "Microsoft MPI version already installed"
      ${EndIf}
  ${EndIf}
 

  # Show debug information
  !ifdef DEBUG
    Messagebox MB_OK "MS-MPI: needed=$MPIInstallationNeeded required=${MPIRequiredVersion} installed=$MPIInstalledVersion"
  !endif

FunctionEnd

# No leave page for required software
Function RequirementsPageLeave
FunctionEnd
