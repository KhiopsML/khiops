file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(KNITransfer ${cppfiles})
set_khiops_options(KNITransfer)

target_link_libraries(KNITransfer PUBLIC KhiopsNativeInterface KWLearningProblem DTForest KMDRRuleLibrary
                                         PLParallelTask $<$<BOOL:${MPI}>:PLMPI>)
