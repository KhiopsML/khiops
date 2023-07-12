file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# on fedora, binaries built with mpi must ended by _mpich suffix
add_executable(MODL_Coclustering${MPI_SUFFIX} ${cppfiles} MODL_Coclustering.rc)
set_khiops_options(MODL_Coclustering${MPI_SUFFIX})

target_link_libraries(MODL_Coclustering${MPI_SUFFIX} PUBLIC KWLearningProblem DTForest KMDRRuleLibrary)

if(MSVC)
  target_link_options(MODL_Coclustering${MPI_SUFFIX} PUBLIC "/INCREMENTAL")
endif()

add_library(MODL_Coclustering_SO SHARED ${cppfiles})
target_link_libraries(MODL_Coclustering_SO PUBLIC KMDRRuleLibrary DTForest KWLearningProblem)
set_target_properties(
  MODL_Coclustering_SO
  PROPERTIES PUBLIC_HEADER MODL_Coclustering_dll.h
             SOVERSION ${PROJECT_VERSION_MAJOR}
             VERSION ${PROJECT_VERSION})
set_khiops_options(MODL_Coclustering_SO)
