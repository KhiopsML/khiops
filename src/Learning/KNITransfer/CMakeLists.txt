file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(KNITransfer ${cppfiles})
set_khiops_options(KNITransfer)

if(MPI)
  target_link_libraries(
    KNITransfer
    PUBLIC KhiopsNativeInterface
           KWLearningProblem
           DTForest
           KMDRRuleLibrary
           PLParallelTask
           PLMPI
           ${MPI_CXX_LIBRARIES})
else()

  target_link_libraries(KNITransfer PUBLIC KhiopsNativeInterface KWLearningProblem DTForest KMDRRuleLibrary
                                           PLParallelTask)
endif()
