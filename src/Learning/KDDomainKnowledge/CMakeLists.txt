file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KDDomainKnowledge STATIC "${cppfiles}")
set_khiops_options(KDDomainKnowledge)
# cmake-format: off
add_view_generation(KDDomainKnowledge KDTextFeatureSpec "Text feature parameters" ../KWUserInterface/KDTextFeatureSpec.dd KDTextFeatureSpec.dd.log -noview)
# cmake-format: on
target_include_directories(KDDomainKnowledge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KDDomainKnowledge PUBLIC KWDataPreparation)
