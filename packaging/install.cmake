# ######################################## Installation #########################################

# ######################################## KNI installation

# Specification of the paths according to the OS
if(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
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

if(UNIX)

  # replace MPIEXEC MPIEXEC_NUMPROC_FLAG and MPI_IMPL KHIOPS_MPI_EXTRA_FLAG ADDITIONAL_EN_VAR
  if("${MPI_IMPL}" STREQUAL "openmpi")
    set(KHIOPS_MPI_EXTRA_FLAG "--allow-run-as-root --quiet")
    set(ADDITIONAL_EN_VAR "export OMPI_MCA_btl_vader_single_copy_mechanism=none # issue on docker")
    if(IS_FEDORA_LIKE)
      set(ADDITIONAL_EN_VAR "${ADDITIONAL_EN_VAR}\nexport PSM3_DEVICES=self # issue one rocky linux")
    endif()
  endif()

  # Add header comment to the variable definition (if any variable is defined)
  if(ADDITIONAL_EN_VAR)
    set(ADDITIONAL_EN_VAR "# Additional variables for MPI\n${ADDITIONAL_EN_VAR}")
  endif()

  configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops-env.in ${TMP_DIR}/khiops-env @ONLY
                 NEWLINE_STYLE UNIX)
  configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/debian/khiops-core/postinst.in ${TMP_DIR}/postinst @ONLY
                 NEWLINE_STYLE UNIX)

  if(NOT IS_FEDORA_LIKE)
    install(TARGETS MODL MODL_Coclustering RUNTIME DESTINATION usr/bin COMPONENT KHIOPS_CORE)

    # We install the binary with mpi suffix and create a symlink without the suffix
    get_target_property(MODL_NAME MODL OUTPUT_NAME)
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink /usr/bin/${MODL_NAME} ${TMP_DIR}/MODL)
    install(
      FILES ${TMP_DIR}/MODL
      DESTINATION usr/bin
      COMPONENT KHIOPS_CORE)
  else()

    # On fedora binaries built with mpi must follow these rules :
    #
    # - the binaries MUST be suffixed with $MPI_SUFFIX
    # - MPI implementation specific files MUST be installed in the directories used by the MPI compiler e.g. $MPI_BIN
    #
    # see https://docs.fedoraproject.org/en-US/packaging-guidelines/MPI/
    #
    install(TARGETS MODL RUNTIME DESTINATION ./${MPI_BIN}/khiops COMPONENT KHIOPS_CORE)
    install(TARGETS MODL_Coclustering RUNTIME DESTINATION /usr/bin COMPONENT KHIOPS_CORE)

    # We install the binary under $MPI_BIN and create a symlink to it
    get_target_property(MODL_NAME MODL OUTPUT_NAME)
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${MPI_BIN}/khiops/${MODL_NAME} ${TMP_DIR}/MODL)
    install(
      FILES ${TMP_DIR}/MODL
      DESTINATION usr/bin
      COMPONENT KHIOPS_CORE)

  endif()

  install(
    PROGRAMS ${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops
             ${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops_coclustering ${TMP_DIR}/khiops-env
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

  install(
    FILES ${CMAKE_BINARY_DIR}/jars/norm.jar ${CMAKE_BINARY_DIR}/jars/khiops.jar
    DESTINATION usr/share/khiops
    COMPONENT KHIOPS)
endif(UNIX)
