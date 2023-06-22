add_executable(PLTest PLTest.cpp)

target_link_libraries(PLTest PUBLIC $<$<BOOL:${MPI}>:PLMPI> PLParallelTask PLSamples)

set_khiops_options(PLTest)
