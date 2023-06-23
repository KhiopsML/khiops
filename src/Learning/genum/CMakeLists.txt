file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(genum ${cppfiles})

set_khiops_options(genum)

target_link_libraries(genum PUBLIC MHHistograms)
set_khiops_options(genum)
