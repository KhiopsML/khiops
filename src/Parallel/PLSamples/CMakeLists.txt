file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(PLSamples STATIC ${cppfiles})

# Set common build options
set_khiops_options(PLSamples)

target_include_directories(PLSamples PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(PLSamples PUBLIC PLParallelTask)
