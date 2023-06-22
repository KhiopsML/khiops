file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(KMDRRuleLibrary STATIC ${cppfiles})
set_khiops_options(KMDRRuleLibrary)

target_include_directories(KMDRRuleLibrary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KMDRRuleLibrary PUBLIC KWUtils KWData KWDataUtils KWDataPreparation KWModeling)
