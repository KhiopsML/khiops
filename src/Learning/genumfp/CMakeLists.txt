file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(genumfp ${cppfiles})

target_link_libraries(genumfp PUBLIC MHHistograms)

set_khiops_options(genumfp)
