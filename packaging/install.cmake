# ######################################## Installation #########################################

set(TMP_DIR ${PROJECT_BINARY_DIR}/tmp)

# API_VERSION_NUMBER is replace in the configure_file process
set(API_VERSION_NUMBER ${PROJECT_VERSION_MAJOR})

configure_file(${PROJECT_SOURCE_DIR}/packaging/common/KNI/PackageRef/samples/C/readme.txt.in ${TMP_DIR}/kni.C.README.txt
               @ONLY NEWLINE_STYLE UNIX)

configure_file(${PROJECT_SOURCE_DIR}/packaging/common/KNI/PackageRef/samples/java/readme.txt.in
               ${TMP_DIR}/kni.java.README.txt @ONLY NEWLINE_STYLE UNIX)

configure_file(${PROJECT_SOURCE_DIR}/packaging/common/KNI/README.txt.in ${TMP_DIR}/kni.README.txt @ONLY
               NEWLINE_STYLE UNIX)

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
  COMPONENT KNI_DOC)

install(
  FILES ${TMP_DIR}/kni.C.README.txt
  DESTINATION ${DOC_DIR}/samples/C
  RENAME README.txt
  COMPONENT KNI_DOC)

install(
  FILES ${TMP_DIR}/kni.java.README.txt
  DESTINATION ${DOC_DIR}/samples/java
  RENAME README.txt
  COMPONENT KNI_DOC)

install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/packaging/common/KNI/PackageRef/samples
  DESTINATION ${DOC_DIR}
  COMPONENT KNI_DOC
  PATTERN "*.in" EXCLUDE
  PATTERN ".*" EXCLUDE)

# Copy KNI c++ files to temporary directory before to add main functions
file(COPY KNITransfer/KNIRecodeFile.cpp DESTINATION ${TMP_DIR}/)
file(COPY KNITransfer/KNIRecodeMTFiles.cpp DESTINATION ${TMP_DIR}/)
file(APPEND ${TMP_DIR}/KNIRecodeFile.cpp
     "\n\nvoid main(int argv, char** argc)\n{\n\tmainKNIRecodeFile(argv, argc);\n}\n")
file(APPEND ${TMP_DIR}/KNIRecodeMTFiles.cpp
     "\n\nvoid main(int argv, char** argc)\n{\n\tmainKNIRecodeMTFiles(argv, argc);\n}\n")

install(
  FILES ${TMP_DIR}/KNIRecodeFile.cpp
  RENAME KNIRecodeFile.c
  DESTINATION ${DOC_DIR}/samples/C
  COMPONENT KNI_DOC)

install(
  FILES ${TMP_DIR}/KNIRecodeMTFiles.cpp
  RENAME KNIRecodeMTFiles.c
  DESTINATION ${DOC_DIR}/samples/C
  COMPONENT KNI_DOC)

install(
  FILES KNITransfer/KNIRecodeFile.h KNITransfer/KNIRecodeMTFiles.h
  DESTINATION ${DOC_DIR}/samples/C
  COMPONENT KNI_DOC)

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
  FILES ${PROJECT_SOURCE_DIR}/packaging/common/khiops/whatsnewV10.1.txt
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
