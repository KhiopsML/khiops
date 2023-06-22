file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(ISDRRuleLibrary STATIC "${cppfiles}")

set_khiops_options(ISDRRuleLibrary)

target_include_directories(ISDRRuleLibrary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ISDRRuleLibrary PUBLIC KWData KWModeling)
