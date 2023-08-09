file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(learning_sample2 ${cppfiles})
set_khiops_options(learning_sample2)
target_link_libraries(learning_sample2 PUBLIC KWLearningProblem)
