file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(norm_sample3 ${cppfiles})
set_khiops_options(norm_sample3)
target_link_libraries(norm_sample3 PUBLIC base)
