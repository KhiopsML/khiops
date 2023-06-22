if(NOT MODL_SO)

  # on fedora, binaries built with mpi must ended by _mpich suffix
  add_executable(
    MODL${MPI_SUFFIX} MDKhiopsLearningProblem.cpp MDKhiopsLearningProblemView.cpp MDKhiopsLearningProject.cpp
                      MDKhiopsModelingSpec.cpp MDKhiopsModelingSpecView.cpp MODL.cpp)

  target_link_libraries(MODL${MPI_SUFFIX} PUBLIC DTForest KMDRRuleLibrary KWLearningProblem $<$<BOOL:${MPI}>:PLMPI>
                                                 PLParallelTask)

  set_khiops_options(MODL${MPI_SUFFIX})

else()
  add_library(MODL_dll SHARED MDKhiopsLearningProblem.cpp MDKhiopsLearningProblemView.cpp MDKhiopsLearningProject.cpp
                              MDKhiopsModelingSpec.cpp MDKhiopsModelingSpecView.cpp MODL_dll.cpp)

  target_link_libraries(MODL_dll PUBLIC DTForest KMDRRuleLibrary KWLearningProblem)

  set_target_properties(
    MODL_dll
    PROPERTIES PUBLIC_HEADER MODL_dll.h
               SOVERSION ${PROJECT_VERSION_MAJOR}
               VERSION ${PROJECT_VERSION})

  set_khiops_options(MODL_dll)

endif(NOT MODL_SO)
