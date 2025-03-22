file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

generate_gui_add_view_get_log_list(ddlogfiles)
add_library(MHHistograms STATIC ${cppfiles} ${ddlogfiles})

set_khiops_options(MHHistograms)
target_include_directories(MHHistograms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MHHistograms KWData KWDataPreparation KWUtils)

# Generate cpp files from dd file
generate_gui_add_view(MHHistogramSpec "Histogram parameters" MHHistogramSpec.dd MHHistogramSpec.dd.log -noarrayview)

# Add dependency to ensure that genere will be built before the gui generation
generate_gui_add_view_add_dependency(MHHistograms)
