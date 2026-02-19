file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(MODL ${cppfiles} MODL.rc)
target_link_libraries(MODL PUBLIC DTForest KWLearningProblem)
set_khiops_options(MODL)

# on fedora, binaries built with mpi must ended by _mpich or _openmpi suffix
if((IS_LINUX OR IS_MACOS) AND MPI)
  set_target_properties(MODL PROPERTIES OUTPUT_NAME "MODL${MPI_SUFFIX}")
endif()

if(MPI)
  target_link_libraries(MODL PUBLIC PLMPI)
endif()

add_library(MODL_DLL SHARED ${cppfiles})
target_link_libraries(MODL_DLL PUBLIC DTForest KWLearningProblem)
if(MPI)
  target_link_libraries(MODL_DLL PUBLIC PLMPI)
endif()

set_target_properties(
  MODL_DLL
  PROPERTIES PUBLIC_HEADER MODL_dll.h
             SOVERSION ${PROJECT_VERSION_MAJOR}
             VERSION ${KHIOPS_VERSION})
set_khiops_options(MODL_DLL)
