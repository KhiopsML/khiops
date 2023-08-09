file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(learning_sample3 ${cppfiles})
set_khiops_options(learning_sample3)
# Generate cpp view files from dd files
# cmake-format: off
add_view_generation(learning_sample3 MYModelingSpec "Modeling parameters" MYModelingSpec.dd MYModelingSpec.dd.log -super KWModelingSpec -noarrayview)
add_view_generation(learning_sample3 MYAnalysisResults "Analysis results" MYAnalysisResults.dd MYAnalysisResults.dd.log -super KWAnalysisResults -noarrayview)
# cmake-format: on
target_link_libraries(learning_sample3 PUBLIC KWLearningProblem)
