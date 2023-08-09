file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(generetest ${cppfiles})
set_khiops_options(generetest)
add_view_generation(generetest Sample "Sample" sample.dd sample.dd.log)
target_link_libraries(generetest PUBLIC base)
