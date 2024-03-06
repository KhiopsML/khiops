file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(norm_sample1 ${cppfiles})
set_khiops_options(norm_sample1)
target_link_libraries(norm_sample1 PUBLIC base)
