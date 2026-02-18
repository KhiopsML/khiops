# ######################################## Installation #########################################

# ######################################## KNI installation

# Specification of the paths according to the OS
if(IS_WINDOWS)
  set(INCLUDE_DIR include)
  set(LIB_DIR lib)
  set(DOC_DIR "./")
else()
  set(INCLUDE_DIR usr/include)
  set(LIB_DIR usr/lib)
  set(DOC_DIR usr/share/doc/kni)
endif()

install(
  TARGETS KhiopsNativeInterface
  LIBRARY DESTINATION ${LIB_DIR} COMPONENT KNI
  PUBLIC_HEADER DESTINATION ${INCLUDE_DIR} COMPONENT KNI
  ARCHIVE COMPONENT KNI # lib on windows
  RUNTIME COMPONENT KNI # dll on windows
)

install(
  FILES ${PROJECT_SOURCE_DIR}/LICENSE
  DESTINATION ${DOC_DIR}
  COMPONENT KNI)

install(
  FILES ${TMP_DIR}/kni.README.txt
  DESTINATION ${DOC_DIR}
  COMPONENT KNI
  RENAME README.txt)

# Copy KNI c++ files to temporary directory before to add main functions
configure_file(${PROJECT_SOURCE_DIR}/src/Learning/KNITransfer/KNIRecodeFile.cpp ${TMP_DIR}/KNIRecodeFile.c COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/src/Learning/KNITransfer/KNIRecodeMTFiles.cpp ${TMP_DIR}/KNIRecodeMTFiles.c
               COPYONLY)

file(APPEND ${TMP_DIR}/KNIRecodeFile.c
     "\n\nint main(int argv, char** argc)\n{\n\tmainKNIRecodeFile(argv, argc);\n \treturn 0;\n}\n")
file(APPEND ${TMP_DIR}/KNIRecodeMTFiles.c
     "\n\nint main(int argv, char** argc)\n{\n\tmainKNIRecodeMTFiles(argv, argc);\n \treturn 0;\n}\n")

# Replace PROJECT_VERSION, KHIOPS_VERSION and scripts

file(READ ${PROJECT_SOURCE_DIR}/packaging/common/KNI/build-c-linux.sh BUILD_C_LINUX)
file(READ ${PROJECT_SOURCE_DIR}/packaging/common/KNI/build-c-windows.cmd BUILD_C_WINDOWS)
file(READ ${PROJECT_SOURCE_DIR}/packaging/common/KNI/build-java.sh BUILD_JAVA)
file(READ ${PROJECT_SOURCE_DIR}/packaging/common/KNI/run-java-linux.sh RUN_JAVA_LINUX)
file(READ ${PROJECT_SOURCE_DIR}/packaging/common/KNI/run-java-windows.cmd RUN_JAVA_WINDOWS)

configure_file(${PROJECT_SOURCE_DIR}/packaging/common/KNI/README.txt.in ${TMP_DIR}/kni.README.txt @ONLY
               NEWLINE_STYLE UNIX)
configure_file(${PROJECT_SOURCE_DIR}/packaging/common/KNI/template-README.md ${TMP_DIR}/kni.README.md @ONLY
               NEWLINE_STYLE UNIX)

# ######################################## KNITransfer installation

if(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  install(TARGETS KNITransfer RUNTIME DESTINATION "./" COMPONENT KNI_TRANSFER)
else()
  install(TARGETS KNITransfer RUNTIME DESTINATION usr/bin COMPONENT KNI_TRANSFER)
endif()

# ######################################## Khiops and Khiops Coclustering installation

if(IS_LINUX OR IS_MACOS)

  # Set khiops and khiops_coclustering paths according to the environment (conda, fedora, etc)
  if(IS_CONDA)
    set(MODL_PATH "$(get_script_dir)")
    set(GET_PROC_NUMBER_PATH "$(get_script_dir)")
    set(IS_CONDA_VAR "\n# Inside conda environment\nexport _IS_CONDA=true")
    set(SET_KHIOPS_DRIVERS_PATH "\n# Drivers search path\nexport KHIOPS_DRIVERS_PATH=$(dirname $(get_script_dir))/lib")
  else()
    if(IS_FEDORA_LIKE)
      set(MODL_PATH "${MPI_BIN}/khiops/")
      configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/redhat/khiops_env/use_environment_module.sh.in
                     ${TMP_DIR}/use_environment_module.sh @ONLY NEWLINE_STYLE UNIX)
      file(READ ${TMP_DIR}/use_environment_module.sh USE_ENVIRONMENT_MODULE)
    else()
      set(MODL_PATH "/usr/bin/")
      set(USE_ENVIRONMENT_MODULE "")
    endif(IS_FEDORA_LIKE)
    set(GET_PROC_NUMBER_PATH "/usr/bin/")

    file(READ ${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops_env/java_settings.sh KHIOPS_JAVA_SETTINGS)

  endif(IS_CONDA)

  # replace MPIEXEC MPIEXEC_NUMPROC_FLAG and MPI_IMPL MPI_EXTRA_FLAG ADDITIONAL_ENV_VAR
  if("${MPI_IMPL}" STREQUAL "openmpi")
    set(MPI_EXTRA_FLAG "--allow-run-as-root")
    # Only decide on applying / not applying the --quiet flag for OpenMPI, i.e. for Linux setups. Note that testing on
    # CMAKE_CROSSCOMPILING is not recommended for projects that target MacOS systems
    # (https://cmake.org/cmake/help/latest/variable/CMAKE_CROSSCOMPILING.html#variable:CMAKE_CROSSCOMPILING)
    if(NOT IS_MACOS)
      string(REPLACE " " ";" MPI_LIBRARY_VERSION_LIST "${MPI_CXX_LIBRARY_VERSION_STRING}")
      list(FIND MPI_LIBRARY_VERSION_LIST "ident:" MPI_LIBRARY_VERSION_PRE_INDEX)
      if(MPI_LIBRARY_VERSION_PRE_INDEX GREATER -1)
        math(EXPR MPI_LIBRARY_VERSION_INDEX "${MPI_LIBRARY_VERSION_PRE_INDEX} + 1")
        list(GET MPI_LIBRARY_VERSION_LIST "${MPI_LIBRARY_VERSION_INDEX}" MPI_LIBRARY_VERSION)
        string(REPLACE "." ";" MPI_LIBRARY_VERSION_SUBLIST "${MPI_LIBRARY_VERSION}")
        list(GET MPI_LIBRARY_VERSION_SUBLIST 0 MPI_LIBRARY_VERSION_MAJOR)
        if("${MPI_LIBRARY_VERSION_MAJOR}" LESS 5)
          set(KHIOPS_MPI_QUIET "--quiet")
        endif()
      endif()
    endif(NOT IS_MACOS)
    set(ADDITIONAL_ENV_VAR "export OMPI_MCA_btl_vader_single_copy_mechanism=none # issue on docker")
    set(ADDITIONAL_ENV_VAR_DISPLAY
        "    echo OMPI_MCA_btl_vader_single_copy_mechanism \"$OMPI_MCA_btl_vader_single_copy_mechanism\"")
    if(IS_FEDORA_LIKE)
      set(ADDITIONAL_ENV_VAR "${ADDITIONAL_ENV_VAR}\nexport PSM3_DEVICES=self # issue on rocky linux")
      set(ADDITIONAL_ENV_VAR_DISPLAY "${ADDITIONAL_ENV_VAR_DISPLAY}\n    echo PSM3_DEVICES \"$PSM3_DEVICES\"")
    endif()
  elseif("${MPI_IMPL}" STREQUAL "mpich")
    # Set localhost on MacOS (see issue # https://github.com/pmodels/mpich/issues/4710)
    if(IS_MACOS)
      set(MPI_EXTRA_FLAG "-host localhost")
    endif(IS_MACOS)
  endif()

  # Add header comment to the variable definition (if any variable is defined)
  if(ADDITIONAL_ENV_VAR)
    set(ADDITIONAL_ENV_VAR "\n# Additional variables for MPI\n${ADDITIONAL_ENV_VAR}")
    set(ADDITIONAL_ENV_VAR_DISPLAY "\n    # Additional variables for MPI\n${ADDITIONAL_ENV_VAR_DISPLAY}")
  endif()

  # Get the real file name of MODL e.g MODL_openmpi
  get_target_property(MODL_NAME MODL OUTPUT_NAME)
  get_target_property(MODL_COCLUSTERING_NAME MODL_Coclustering OUTPUT_NAME)

  # For all mpi implementation except openmpi, we compute the proc number (with openmpi, the -n flag is not mandatory)
  if(NOT "${MPI_IMPL}" STREQUAL "openmpi")
    # Replace the path of _khiopsgetprocnumber in set_proc_number.in (with the variable GET_PROC_NUMBER_PATH)
    configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops_env/set_proc_number.in
                   ${TMP_DIR}/set_proc_number.sh @ONLY NEWLINE_STYLE UNIX)
    # The variable SET_PROC_NUMBER is filled with the content of set_proc_number.sh This variable will be replaced in
    # khiops_env.in with configure_file
    file(READ ${TMP_DIR}/set_proc_number.sh SET_PROC_NUMBER)

    # Add _khiopsgetprocnumber to the khiops_core package except for openmpi
    install(TARGETS _khiopsgetprocnumber RUNTIME DESTINATION ./${GET_PROC_NUMBER_PATH} COMPONENT KHIOPS_CORE)
  endif()

  configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops_env/khiops_env.in ${TMP_DIR}/khiops_env @ONLY
                 NEWLINE_STYLE UNIX)
  configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/debian/khiops-core/postinst.in ${TMP_DIR}/postinst @ONLY
                 NEWLINE_STYLE UNIX)
  set(KHIOPS_BINARY_PATH "$KHIOPS_PATH")
  set(TOOL_EXT "_kh")
  configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops.in ${TMP_DIR}/khiops @ONLY NEWLINE_STYLE UNIX)
  set(KHIOPS_BINARY_PATH "$KHIOPS_COCLUSTERING_PATH")
  set(TOOL_EXT "_khc")
  configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops.in ${TMP_DIR}/khiops_coclustering @ONLY
                 NEWLINE_STYLE UNIX)

  install(TARGETS MODL MODL_Coclustering RUNTIME DESTINATION ${MODL_PATH} COMPONENT KHIOPS_CORE)

  install(
    PROGRAMS ${TMP_DIR}/khiops ${TMP_DIR}/khiops_coclustering ${TMP_DIR}/khiops_env
    DESTINATION usr/bin
    COMPONENT KHIOPS_CORE)

  install(
    FILES ${PROJECT_SOURCE_DIR}/LICENSE
    DESTINATION usr/share/doc/khiops
    COMPONENT KHIOPS_CORE)

  install(
    FILES ${PROJECT_SOURCE_DIR}/packaging/common/khiops/WHATSNEW.txt
          ${PROJECT_SOURCE_DIR}/packaging/common/khiops/README.txt
    DESTINATION usr/share/doc/khiops
    COMPONENT KHIOPS)

  install(
    FILES ${PROJECT_SOURCE_DIR}/packaging/common/images/khiops.png
          ${PROJECT_SOURCE_DIR}/packaging/common/images/khiops_coclustering.png
    DESTINATION usr/share/pixmaps
    COMPONENT KHIOPS)

  install(
    FILES ${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops.desktop
          ${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops-coclustering.desktop
    DESTINATION usr/share/applications
    COMPONENT KHIOPS)

  if(BUILD_JARS)
    install_jar(
      khiops_jar
      DESTINATION usr/share/khiops
      COMPONENT KHIOPS)
    install_jar(
      norm_jar
      DESTINATION usr/share/khiops
      COMPONENT KHIOPS)
  endif()

else(IS_LINUX OR IS_MACOS)

  if(IS_CONDA)
    set(GUI_STATUS "false")
    set(SET_MPI "SET_MPI_CONDA")
    set(IS_CONDA_VAR "REM Inside conda environment\r\nset \"_IS_CONDA=true\"")
    set(SET_KHIOPS_DRIVERS_PATH "REM Drivers search path\r\nset \"KHIOPS_DRIVERS_PATH=%_KHIOPS_HOME%\\bin\"")
  else()
    set(SET_MPI "SET_MPI_SYSTEM_WIDE")
    set(GUI_STATUS "true")
  endif()

  configure_file(${PROJECT_SOURCE_DIR}/packaging/windows/khiops_env.cmd.in ${TMP_DIR}/khiops_env.cmd @ONLY
                 NEWLINE_STYLE CRLF)

  # khiops.cmd and khiops_coclustering.cmd differ only with the binary path: KHIOPS_PATH or KHIOPS_COCLUSTRING_PATH.
  # They both build from the same file: khiops.cmd.in
  set(MODL_PATH "KHIOPS_PATH")
  set(TOOL_NAME "Khiops")
  set(TOOL_EXT "_kh")
  configure_file(${PROJECT_SOURCE_DIR}/packaging/windows/khiops.cmd.in ${TMP_DIR}/khiops.cmd @ONLY NEWLINE_STYLE CRLF)
  set(MODL_PATH "KHIOPS_COCLUSTERING_PATH")
  set(TOOL_NAME "Khiops_coclustering")
  set(TOOL_EXT "_khc")
  configure_file(${PROJECT_SOURCE_DIR}/packaging/windows/khiops.cmd.in ${TMP_DIR}/khiops_coclustering.cmd @ONLY
                 NEWLINE_STYLE CRLF)

  install(TARGETS MODL MODL_Coclustering _khiopsgetprocnumber RUNTIME DESTINATION bin COMPONENT KHIOPS_CORE)

  install(
    PROGRAMS ${TMP_DIR}/khiops.cmd ${TMP_DIR}/khiops_coclustering.cmd ${TMP_DIR}/khiops_env.cmd
    DESTINATION bin
    COMPONENT KHIOPS_CORE)

  if(BUILD_JARS)
    install_jar(
      khiops_jar
      DESTINATION jars
      COMPONENT KHIOPS)
    install_jar(
      norm_jar
      DESTINATION jars
      COMPONENT KHIOPS)
  endif()

endif(IS_LINUX OR IS_MACOS)

# ######################################## khisto installation
if(IS_PIP)
  install(TARGETS khisto RUNTIME DESTINATION ${SKBUILD_SCRIPTS_DIR} COMPONENT KHISTO)
elseif(IS_LINUX)
  install(TARGETS khisto RUNTIME DESTINATION usr/bin COMPONENT KHISTO)
  install(
    FILES ${PROJECT_SOURCE_DIR}/LICENSE
    DESTINATION usr/share/doc/khisto
    COMPONENT KHISTO)
elseif(IS_WINDOWS)
  install(TARGETS khisto RUNTIME DESTINATION bin COMPONENT KHISTO)
  install(
    FILES ${PROJECT_SOURCE_DIR}/LICENSE
    DESTINATION doc
    COMPONENT KHISTO)
endif()
