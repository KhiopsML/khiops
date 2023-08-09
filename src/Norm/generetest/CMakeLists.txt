file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

generate_gui_add_view_get_log_list(ddlogfiles)
add_executable(generetest ${cppfiles})
if(GENERATE_VIEWS)
  target_sources(generetest PRIVATE ${ddlogfiles})
endif()

set_khiops_options(generetest)
target_link_libraries(generetest PUBLIC base)

# Generate cpp files from dd file
generate_gui_add_view(Sample "Sample" sample.dd sample.dd.log)

# Add dependency to ensure that genere will be built before the gui generation
generate_gui_add_view_add_dependency(generetest)
