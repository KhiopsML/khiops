file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
generate_gui_add_view_get_log_list(ddlogfiles)

add_executable(learning_sample3 ${cppfiles} ${ddlogfiles})
set_khiops_options(learning_sample3)
target_link_libraries(learning_sample3 PUBLIC KWLearningProblem)

# Generate cpp files from dd file
# cmake-format: off
 generate_gui_add_view(MYModelingSpec "Modeling parameters" MYModelingSpec.dd MYModelingSpec.dd.log -super KWModelingSpec -noarrayview)
 generate_gui_add_view(MYAnalysisResults "Analysis results" MYAnalysisResults.dd MYAnalysisResults.dd.log -super KWAnalysisResults -noarrayview)
# cmake-format: on

# Add dependency to ensure that genere will be built before the gui generation
generate_gui_add_view_add_dependency(learning_sample3)
