file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

if(NOT MODL_SO)
  # on fedora, binaries built with mpi must ended by _mpich suffix is set with -DMPI_SUFFIX

  add_executable(MODL_Coclustering ${cppfiles})

  set_khiops_options(MODL_Coclustering)

  target_link_libraries(MODL_Coclustering PUBLIC KWLearningProblem DTForest KMDRRuleLibrary)

  if(MSVC)
    target_link_options(MODL_Coclustering PUBLIC "/INCREMENTAL")
  endif()

else(NOT MODL_SO)
  add_library(MODL_Coclustering SHARED ${cppfiles})

  target_link_libraries(MODL_Coclustering PUBLIC KWLearningProblem DTForest KMDRRuleLibrary PLParallelTask)

  set_target_properties(
    MODL_Coclustering
    PROPERTIES PUBLIC_HEADER MODL_Coclustering_dll.h
               SOVERSION ${PROJECT_VERSION_MAJOR}
               VERSION ${PROJECT_VERSION})

  set_khiops_options(MODL_Coclustering)
endif(NOT MODL_SO)
