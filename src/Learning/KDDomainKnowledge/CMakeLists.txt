file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KDDomainKnowledge STATIC "${cppfiles}")

if(GENERATE_VIEWS)
  target_sources(KDDomainKnowledge PRIVATE KDTextFeatureSpec.dd.log)
endif()

set_khiops_options(KDDomainKnowledge)

target_include_directories(KDDomainKnowledge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KDDomainKnowledge PUBLIC KWDataPreparation)

generate_gui_add_view(KDTextFeatureSpec "Text feature parameters" ../KWUserInterface/KDTextFeatureSpec.dd
                      KDTextFeatureSpec.dd.log -noview)

generate_gui_add_view_add_dependency(KDDomainKnowledge)
