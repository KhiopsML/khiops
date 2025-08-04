file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(KWTest ${cppfiles})
set_khiops_options(KWTest)
target_link_libraries(KWTest PUBLIC DTForest KWLearningProblem)

if(MPI)
  target_link_libraries(KWTest PUBLIC PLMPI)
endif()
