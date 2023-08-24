file(GLOB cppfiles *.cpp)
add_executable(parallel_mpi_test ${cppfiles})
set_khiops_options(parallel_mpi_test)
target_compile_definitions(parallel_mpi_test PRIVATE USE_MPI)
target_compile_definitions(parallel_mpi_test PUBLIC MPI_DEV)
target_link_libraries(
  parallel_mpi_test
  GTest::gtest_main
  PLParallelTask
  PLSamples
  PLMPI
  ${MPI_CXX_LIBRARIES}
  testutils)
target_include_directories(parallel_mpi_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MPI_CXX_INCLUDE_PATH})
target_compile_options(parallel_mpi_test PUBLIC ${GTEST_CFLAGS})
include(GoogleTest)
gtest_discover_tests(parallel_mpi_test)
