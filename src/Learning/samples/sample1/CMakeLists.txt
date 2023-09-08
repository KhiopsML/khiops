file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(learning_sample1 ${cppfiles})
set_khiops_options(learning_sample1)
target_link_libraries(learning_sample1 PUBLIC KWLearningProblem)
