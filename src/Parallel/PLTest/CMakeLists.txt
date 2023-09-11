add_executable(PLTest PLTest.cpp)

target_link_libraries(PLTest PUBLIC PLParallelTask PLSamples)
if(MPI)
  target_link_libraries(PLTest PUBLIC PLMPI ${MPI_CXX_LIBRARIES})
endif()

set_khiops_options(PLTest)
