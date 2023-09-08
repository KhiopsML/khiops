file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

generate_gui_add_view_get_log_list(ddlogfiles)
add_executable(norm_sample2 ${cppfiles} ${ddlogfiles})
set_khiops_options(norm_sample2)
target_link_libraries(norm_sample2 PUBLIC base)

# Generate cpp files from dd file
generate_gui_add_view(PRWorker "Employe" PRWorker.dd PRWorker.dd.log -noarrayview)

# Add dependency to ensure that genere will be built before the gui generation
generate_gui_add_view_add_dependency(norm_sample2)
