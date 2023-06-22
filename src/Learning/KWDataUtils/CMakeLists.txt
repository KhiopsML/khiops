file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KWDataUtils STATIC "${cppfiles}")
set_khiops_options(KWDataUtils)

target_include_directories(KWDataUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KWDataUtils PUBLIC KWDRRuleLibrary)
