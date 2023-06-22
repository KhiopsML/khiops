file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KWUserInterface STATIC "${cppfiles}")
set_khiops_options(KWUserInterface)

target_include_directories(KWUserInterface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(KWUserInterface PUBLIC KDDomainKnowledge KWDataPreparation KWModeling)
