file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(khisto ${cppfiles} khisto.rc)
set_khiops_options(khisto)
target_link_libraries(khisto PUBLIC MHHistograms)
