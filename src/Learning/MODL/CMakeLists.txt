file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# on fedora, binaries built with mpi must ended by _mpich suffix
add_executable(MODL${MPI_SUFFIX} ${cppfiles} MODL.rc)
target_link_libraries(MODL${MPI_SUFFIX} PUBLIC DTForest KMDRRuleLibrary KWLearningProblem)
set_khiops_options(MODL${MPI_SUFFIX})
if(MPI)
  target_link_libraries(MODL${MPI_SUFFIX} PUBLIC PLMPI)
endif()

add_library(MODL_DLL SHARED ${cppfiles})
target_link_libraries(MODL_DLL PUBLIC DTForest KMDRRuleLibrary KWLearningProblem)
if(MPI)
  target_link_libraries(MODL_DLL PUBLIC PLMPI)
endif()

set_target_properties(
  MODL_DLL
  PROPERTIES PUBLIC_HEADER MODL_dll.h
             SOVERSION ${PROJECT_VERSION_MAJOR}
             VERSION ${PROJECT_VERSION})
set_khiops_options(MODL_DLL)
