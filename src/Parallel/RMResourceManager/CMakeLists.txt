file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(RMResourceManager STATIC ${cppfiles})

set_khiops_options(RMResourceManager)

target_include_directories(RMResourceManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(RMResourceManager PUBLIC base)
