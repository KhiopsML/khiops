file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KDDomainKnowledge STATIC "${cppfiles}")

set_khiops_options(KDDomainKnowledge)

target_include_directories(KDDomainKnowledge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KDDomainKnowledge PUBLIC KWDataPreparation)
