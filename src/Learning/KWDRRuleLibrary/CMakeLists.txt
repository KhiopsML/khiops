file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KWDRRuleLibrary STATIC "${cppfiles}")
set_khiops_options(KWDRRuleLibrary)

target_include_directories(KWDRRuleLibrary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KWDRRuleLibrary PUBLIC KWData KWDataPreparation KWModeling)
