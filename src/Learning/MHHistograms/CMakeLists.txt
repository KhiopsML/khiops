file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(MHHistograms STATIC ${cppfiles})
set_khiops_options(MHHistograms)
target_include_directories(MHHistograms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MHHistograms KWData KWDataPreparation KWUtils)
