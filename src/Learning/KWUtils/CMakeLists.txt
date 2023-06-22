file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(KWUtils STATIC ${cppfiles})
set_khiops_options(KWUtils)

target_include_directories(KWUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KWUtils base)
