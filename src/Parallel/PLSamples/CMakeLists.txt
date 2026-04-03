file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
generate_gui_add_view_get_log_list(ddlogfiles)
add_library(PLSamples STATIC ${cppfiles} ${ddlogfiles})

# Set common build options
set_khiops_options(PLSamples)

target_include_directories(PLSamples PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(PLSamples PUBLIC PLParallelTask)

# Generate cpp files from dd file
generate_gui_add_view(PEPi "Pi parallel computation" PEPi.dd PEPi.dd.log -noarrayview)

# Add dependency to ensure that genere will be built before the gui generation
generate_gui_add_view_add_dependency(PLSamples)

# Add subdirectory of the Pi computation subproject
add_subdirectory(compute_pi)
