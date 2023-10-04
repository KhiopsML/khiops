# ######################################## Installation #########################################

set(TMP_DIR ${PROJECT_BINARY_DIR}/tmp)

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
  RENAME README.txt
  COMPONENT KNI)

# Copy KNI c++ files to temporary directory before to add main functions
configure_file(${PROJECT_SOURCE_DIR}/src/Learning/KNITransfer/KNIRecodeFile.cpp ${TMP_DIR}/KNIRecodeFile.c COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/src/Learning/KNITransfer/KNIRecodeMTFiles.cpp ${TMP_DIR}/KNIRecodeMTFiles.c
               COPYONLY)

file(APPEND ${TMP_DIR}/KNIRecodeFile.c
     "\n\nint main(int argv, char** argc)\n{\n\tmainKNIRecodeFile(argv, argc);\n \treturn 0;\n}\n")
file(APPEND ${TMP_DIR}/KNIRecodeMTFiles.c
     "\n\nint main(int argv, char** argc)\n{\n\tmainKNIRecodeMTFiles(argv, argc);\n \treturn 0;\n}\n")

# Replace PROJECT_VERSION and scripts

file(READ ${PROJECT_SOURCE_DIR}/packaging/common/KNI/build-c-linux.sh BUILD_C_LINUX)
file(READ ${PROJECT_SOURCE_DIR}/packaging/common/KNI/build-c-windows.cmd BUILD_C_WINDOWS)
file(READ ${PROJECT_SOURCE_DIR}/packaging/common/KNI/build-java.sh BUILD_JAVA)
file(READ ${PROJECT_SOURCE_DIR}/packaging/common/KNI/run-java-linux.sh RUN_JAVA_LINUX)
file(READ ${PROJECT_SOURCE_DIR}/packaging/common/KNI/run-java-windows.sh RUN_JAVA_WINDOWS)

configure_file(${PROJECT_SOURCE_DIR}/packaging/common/KNI/README.txt.in ${TMP_DIR}/kni.README.txt @ONLY
               NEWLINE_STYLE UNIX)
configure_file(${PROJECT_SOURCE_DIR}/packaging/common/KNI/template-README.md ${TMP_DIR}/kni.README.md @ONLY
               NEWLINE_STYLE UNIX)

# ######################################## Khiops and Khiops Coclustering installation

if(NOT IS_FEDORA_LIKE)
  install(TARGETS MODL MODL_Coclustering RUNTIME DESTINATION usr/bin COMPONENT KHIOPS_CORE)
else()

  # On fedora binaries built with mpi must follow these rules :
  #
  # - the binaries MUST be suffixed with $MPI_SUFFIX
  # - MPI implementation specific files MUST be installed in the directories used by the used MPI compiler e.g. $MPI_BIN
  #
  # see https://docs.fedoraproject.org/en-US/packaging-guidelines/MPI/
  #
  install(TARGETS MODL${MPI_SUFFIX} RUNTIME DESTINATION ./${MPI_BIN}/khiops COMPONENT KHIOPS_CORE)
  install(TARGETS MODL_Coclustering${MPI_SUFFIX} RUNTIME DESTINATION ./${MPI_BIN}/khiops COMPONENT KHIOPS_CORE)

  # We install the binary under $MPI_BIN and create a symlink to it
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${MPI_BIN}/khiops/MODL${MPI_SUFFIX}
                          ${CMAKE_BINARY_DIR}/MODL)
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${MPI_BIN}/khiops/MODL_Coclustering${MPI_SUFFIX}
                          ${CMAKE_BINARY_DIR}/MODL_Coclustering)

  install(
    FILES ${CMAKE_BINARY_DIR}/MODL ${CMAKE_BINARY_DIR}/MODL_Coclustering
    DESTINATION usr/bin
    COMPONENT KHIOPS_CORE)

endif()

install(
  PROGRAMS ${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops
           ${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops_coclustering
           ${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops-env
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
