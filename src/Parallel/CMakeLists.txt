project(Parallel LANGUAGES CXX)

add_subdirectory(RMResourceManager)
add_subdirectory(PLParallelTask)

if(MPI)
  add_subdirectory(PLMPI)
endif(MPI)
add_subdirectory(PLSamples)
if(FULL)
  add_subdirectory(PLTest)
endif(FULL)
