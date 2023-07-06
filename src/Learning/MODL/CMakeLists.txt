# on fedora, binaries built with mpi must ended by _mpich suffix
add_executable(MODL${MPI_SUFFIX} MDKhiopsLearningProblem.cpp MDKhiopsLearningProblemView.cpp
                                 MDKhiopsLearningProject.cpp MODL.cpp)

target_link_libraries(MODL${MPI_SUFFIX} PUBLIC DTForest KMDRRuleLibrary KWLearningProblem)

set_khiops_options(MODL${MPI_SUFFIX})

add_library(MODL_SO SHARED MDKhiopsLearningProblem.cpp MDKhiopsLearningProblemView.cpp MDKhiopsLearningProject.cpp
                           MODL_dll.cpp)

target_link_libraries(MODL_SO PUBLIC DTForest KMDRRuleLibrary KWLearningProblem)

if(MPI)
  target_link_libraries(MODL PUBLIC PLMPI)
  target_link_libraries(MODL_SO PUBLIC PLMPI)
endif()

set_target_properties(
  MODL_SO
  PROPERTIES PUBLIC_HEADER MODL_dll.h
             SOVERSION ${PROJECT_VERSION_MAJOR}
             VERSION ${PROJECT_VERSION})

set_khiops_options(MODL_SO)
