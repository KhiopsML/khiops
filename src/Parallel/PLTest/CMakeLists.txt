add_executable(PLTest PLTest.cpp)

if(MPI)
  target_link_libraries(PLTest PUBLIC PLParallelTask PLSamples PLMPI ${MPI_CXX_LIBRARIES})
else()
  target_link_libraries(PLTest PUBLIC PLParallelTask PLSamples)
endif()

set_khiops_options(PLTest)
