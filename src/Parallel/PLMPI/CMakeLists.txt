file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Create target
add_library(PLMPI STATIC ${cppfiles})

# Set common build options
set_khiops_options(PLMPI)

# findMPI fails on windows with clang, we add our own findMPI if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
# CMAKE_SYSTEM_NAME STREQUAL "Windows" ) if(NOT DEFINED ENV{MSMPI_INC}) message(FATAL "MSMPI is not installed") endif()
# set(MPI_CXX_INCLUDE_PATH $ENV{MSMPI_INC}) set(MPI_CXX_LIBRARIES $ENV{MSMPI_LIB64}/msmpi.lib) #TODO test architecture
# message("MPI is found at ${MPI_CXX_INCLUDE_PATH}") else() find_package(MPI 2.0 REQUIRED) endif() TODO ckeck MPI is
# package is found
target_include_directories(
  PLMPI
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${MPI_CXX_INCLUDE_PATH})
target_link_libraries(PLMPI PUBLIC PLParallelTask base ${MPI_CXX_LIBRARIES})
target_compile_definitions(PLMPI PRIVATE MPI_DEV)
