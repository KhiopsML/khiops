add_executable(PLTest PLTest.cpp)

target_link_libraries(PLTest PUBLIC PLParallelTask PLSamples)

set_khiops_options(PLTest)
