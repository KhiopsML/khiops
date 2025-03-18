file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
generate_gui_add_view_get_log_list(ddlogfiles)

add_library(KIInterpretation STATIC "${cppfiles}" "${ddlogfiles}")

target_include_directories(KIInterpretation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KIInterpretation PUBLIC KDDomainKnowledge KWDataPreparation KWModeling KWUserInterface)

set_khiops_options(KIInterpretation)

# cmake-format: off
# Generate cpp files from dd file
 generate_gui_add_view(KIModelService "Interpretation service" KIModelService.dd KIModelService.dd.log -noarrayview)
 generate_gui_add_view(KIModelInterpreter "Interpret model" KIModelInterpreter.dd KIModelInterpreter.dd.log -noarrayview -super KIModelService)
 generate_gui_add_view(KIModelReinforcer "Reinforce model" KIModelReinforcer.dd KIModelReinforcer.dd.log -noarrayview -super KIModelService)
 generate_gui_add_view(KIPredictorAttribute "Predictor variable" KIPredictorAttribute.dd KIPredictorAttribute.dd.log)
# cmake-format: on

# Add dependency to ensure that genere will be built before the gui generation
generate_gui_add_view_add_dependency(KIInterpretation)
