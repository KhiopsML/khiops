file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(genum ${cppfiles} genum.rc)
# cmake-format: off
add_view_generation(genum MHGenumHistogramSpec "Histogram parameters" MHGenumHistogramSpec.dd MHGenumHistogramSpec.dd.log -noview -noarrayview -super MHHistogramSpec)
# cmake-format: on
set_khiops_options(genum)
target_link_libraries(genum PUBLIC MHHistograms)
