file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KWModeling STATIC "${cppfiles}")
set_khiops_options(KWModeling)
# Generate cpp files from dd file
# cmake-format: off
# La partie vue est geree dans la librairie KWUserInterface
add_view_generation(KWModeling KWTrainParameters "Train parameters" KWTrainParameters.dd KWTrainParameters.dd.noview.log -noview)
add_view_generation(KWModeling KWSelectionParameters "Selection parameters" KWSelectionParameters.dd KWSelectionParameters.dd.log -noview)
add_view_generation(KWModeling KWRecodingSpec "Recoding parameters" ../KWUserInterface/KWRecodingSpec.dd KWRecodingSpec.dd.log -noview)
# cmake-format: on
target_include_directories(KWModeling PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KWModeling PUBLIC KDDomainKnowledge KWDataPreparation)
