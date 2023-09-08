file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
generate_gui_add_view_get_log_list(ddlogfiles)

add_library(KWModeling STATIC "${cppfiles}")

if(GENERATE_VIEWS)
  target_sources(KWModeling PRIVATE "${ddlogfiles}" KWRecodingSpec.dd.log)
endif()

set_khiops_options(KWModeling)

target_include_directories(KWModeling PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KWModeling PUBLIC KDDomainKnowledge KWDataPreparation)

# Generate cpp files from dd file
# cmake-format: off

# La partie vue est geree dans la librairie KWUserInterface
 generate_gui_add_view(KWTrainParameters "Train parameters" KWTrainParameters.dd KWTrainParameters.dd.log -noview)

# La partie vue est geree dans la librairie KWUserInterface
 generate_gui_add_view(KWSelectionParameters "Selection parameters" KWSelectionParameters.dd KWSelectionParameters.dd.log -noview)
 generate_gui_add_view(KWRecodingSpec "Recoding parameters" ../KWUserInterface/KWRecodingSpec.dd KWRecodingSpec.dd.log -noview)

# cmake-format: on

# Add dependency to ensure that genere will be built before the gui generation
generate_gui_add_view_add_dependency(KWModeling)
