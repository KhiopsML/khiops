file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(SNBPredictor STATIC "${cppfiles}")
set_khiops_options(SNBPredictor)

target_include_directories(SNBPredictor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  SNBPredictor
  PUBLIC KWData
         KWDataPreparation
         KWDataUtils
         KWDRRuleLibrary
         KWModeling
         KWUtils
         KWDataUtils
         KWDRRuleLibrary
         KWModeling
         KWUserInterface
         KWUtils)
