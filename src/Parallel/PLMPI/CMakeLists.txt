file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Create target
add_library(PLMPI STATIC ${cppfiles})

# Set common build options
set_khiops_options(PLMPI)

target_include_directories(PLMPI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  PLMPI PUBLIC PLParallelTask base MPI::MPI_CXX) # By using the mpi target MPI_CXX, all is set correctly for mpi
                                                 # (include, libs, compiler flags, ...)
target_compile_definitions(PLMPI PRIVATE MPI_DEV)
