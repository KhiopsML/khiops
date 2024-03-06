file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

generate_gui_add_view_get_log_list(ddlogfiles)
add_executable(genum ${cppfiles} ${ddlogfiles} genum.rc)
set_khiops_options(genum)
target_link_libraries(genum PUBLIC MHHistograms)

# Generate cpp files from dd file
# cmake-format: off
 generate_gui_add_view(MHGenumHistogramSpec "Histogram parameters" MHGenumHistogramSpec.dd MHGenumHistogramSpec.dd.log -noview -noarrayview -super MHHistogramSpec)
# cmake-format: on

# Add dependency to ensure that genere will be built before the gui generation
generate_gui_add_view_add_dependency(genum)
