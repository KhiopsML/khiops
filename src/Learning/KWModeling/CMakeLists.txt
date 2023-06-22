file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KWModeling STATIC "${cppfiles}")
set_khiops_options(KWModeling)

target_include_directories(KWModeling PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KWModeling PUBLIC KDDomainKnowledge KWDataPreparation)
