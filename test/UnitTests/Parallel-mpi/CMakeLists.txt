file(GLOB cppfiles *.cpp)
add_executable(parallel_mpi_test ${cppfiles})
set_khiops_options(parallel_mpi_test)
target_compile_definitions(parallel_mpi_test PRIVATE USE_MPI)
target_compile_definitions(parallel_mpi_test PUBLIC MPI_DEV)
target_link_libraries(parallel_mpi_test PUBLIC GTest::gtest_main PLParallelTask PLSamples testutils)
if(MPI)
  target_link_libraries(parallel_mpi_test PUBLIC PLMPI)
endif()

target_include_directories(parallel_mpi_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(parallel_mpi_test PUBLIC ${GTEST_CFLAGS})
include(GoogleTest)
gtest_discover_tests(parallel_mpi_test)

# Add the same test in CTest framework. It allows to run the test directly with the right mpi command. The command
# "ctest --preset linux-gcc-release -L mpi" will run (depending on the mpi implementation and the core number): mpiexec
# -n 8  build/linux-gcc-release/parallel_mpi_test
add_test(NAME ctest_parallel_mpi_test COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
                                              ${MPIEXEC_PREFLAGS} $<TARGET_FILE:parallel_mpi_test> ${MPIEXEC_POSTFLAGS})

# Add "mpi" label to the test
set_tests_properties(ctest_parallel_mpi_test PROPERTIES LABELS "mpi")
