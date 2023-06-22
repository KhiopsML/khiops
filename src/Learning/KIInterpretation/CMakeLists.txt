file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KIInterpretation STATIC "${cppfiles}")

target_include_directories(KIInterpretation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KIInterpretation PUBLIC KDDomainKnowledge KWDataPreparation KWModeling KWUserInterface)

set_khiops_options(KIInterpretation)

file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
set_target_properties(KIInterpretation PROPERTIES PRIVATE_HEADER "${headers}")
