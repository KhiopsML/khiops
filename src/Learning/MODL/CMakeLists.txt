# on fedora, binaries built with mpi must ended by _mpich suffix
file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(MODL${MPI_SUFFIX} ${cppfiles} MODL.rc)

target_link_libraries(MODL${MPI_SUFFIX} PUBLIC DTForest KMDRRuleLibrary KWLearningProblem)

set_khiops_options(MODL${MPI_SUFFIX})

add_library(MODL_SO SHARED ${cppfiles})

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
