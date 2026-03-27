file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
# Add compute_pi executable
add_executable(compute_pi ${cppfiles})
set_khiops_options(compute_pi)

target_include_directories(compute_pi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(compute_pi PUBLIC PLParallelTask)
target_link_libraries(compute_pi PUBLIC PLMPI)
