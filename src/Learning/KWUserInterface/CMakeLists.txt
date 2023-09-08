file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
generate_gui_add_view_get_log_list(ddlogfiles)

add_library(KWUserInterface STATIC "${cppfiles}")
if(GENERATE_VIEWS)
  target_sources(KWUserInterface PRIVATE "${ddlogfiles}" KWSelectionParameters.dd.log KWTrainParameters.dd.log)
endif()
set_khiops_options(KWUserInterface)

target_include_directories(KWUserInterface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KWUserInterface PUBLIC KDDomainKnowledge KWDataPreparation KWModeling MHHistograms)

# Generate cpp files from dd file

# Dans la plus cas, on ne genere que les classe de type vue, les classe de type modele etant geree par ailleur
# cmake-format: off
 generate_gui_add_view(KWDatabase "Database" KWDatabase.dd KWDatabase.dd.log -nomodel -noarrayview)
 generate_gui_add_view(KWDatabaseSampling "Sampling" KWDatabaseSampling.dd KWDatabaseSampling.dd.log -nomodel -noarrayview -specificmodel KWDatabase)
 generate_gui_add_view(KWDatabaseSelection "Selection" KWDatabaseSelection.dd KWDatabaseSelection.dd.log -nomodel -noarrayview -specificmodel KWDatabase)
 generate_gui_add_view(KWSTDatabaseTextFileData "Data" KWSTDatabaseTextFileData.dd KWSTDatabaseTextFileData.dd.log -nomodel -noarrayview -specificmodel KWSTDatabaseTextFile)
 generate_gui_add_view(KWMTDatabaseTextFileData "Data" KWMTDatabaseTextFileData.dd KWMTDatabaseTextFileData.dd.log -nomodel -noarrayview -specificmodel KWMTDatabaseTextFile)
 generate_gui_add_view(KWMTDatabaseMapping "Multi-table mapping" KWMTDatabaseMapping.dd KWMTDatabaseMapping.dd.log -nomodel)
 generate_gui_add_view(KWClassSpec "Dictionary" KWClassSpec.dd KWClassSpec.dd.log)
 generate_gui_add_view(KWAttributeSpec "Variable" KWAttributeSpec.dd KWAttributeSpec.dd.log)
 generate_gui_add_view(KWAttributeName "Variable" KWAttributeName.dd KWAttributeName.dd.log -nomodel)
 generate_gui_add_view(KWAttributePairName "Variable pair" KWAttributePairName.dd KWAttributePairName.dd.log -nomodel)
 generate_gui_add_view(KWEvaluatedPredictorSpec "Evaluated predictor" KWEvaluatedPredictorSpec.dd KWEvaluatedPredictorSpec.dd.log)
 generate_gui_add_view(KWPreprocessingSpec "Preprocessing parameters" KWPreprocessingSpec.dd KWPreprocessingSpec.dd.log -nomodel -noarrayview)
 generate_gui_add_view(KWPreprocessingAdvancedSpec "Unsupervised parameters" KWPreprocessingAdvancedSpec.dd KWPreprocessingAdvancedSpec.dd.log -nomodel -noarrayview -specificmodel KWPreprocessingSpec)
 generate_gui_add_view(KWDataGridOptimizerParameters "Data Grid optimization" KWDataGridOptimizerParameters.dd KWDataGridOptimizerParameters.dd.log -nomodel -noarrayview)
 generate_gui_add_view(KWBenchmarkSpec "Benchmark" KWBenchmarkSpec.dd KWBenchmarkSpec.dd.log -nomodel)
 generate_gui_add_view(KWLearningBenchmark "Learning benchmark" KWLearningBenchmark.dd KWLearningBenchmark.dd.log -nomodel -noarrayview)
 generate_gui_add_view(KWBenchmarkClassSpec "Benchmark dictionary" KWBenchmarkClassSpec.dd KWBenchmarkClassSpec.dd.log -nomodel -noarrayview)

# La partie modele est geree dans la librairie KWModeling
 generate_gui_add_view(KWRecodingSpec "Recoding parameters" KWRecodingSpec.dd KWRecodingSpec.dd.log -nomodel -noarrayview)
 generate_gui_add_view(KWAttributePairsSpec "Variable pairs parameters" KWAttributePairsSpec.dd KWAttributePairsSpec.dd.log -nomodel -noarrayview)
 generate_gui_add_view(KWAttributeConstructionSpec "Feature engineering parameters" KWAttributeConstructionSpec.dd KWAttributeConstructionSpec.dd.log -nomodel -noarrayview)
 generate_gui_add_view(KWSelectionParameters "Selection parameters" ../KWModeling/KWSelectionParameters.dd KWSelectionParameters.dd.log  -nomodel -noarrayview)
 generate_gui_add_view(KWTrainParameters "Train parameters" ../KWModeling/KWTrainParameters.dd KWTrainParameters.dd.log -nomodel -noarrayview)

# La partie modele est geree dans la librairie KDDomainKnowledge
 generate_gui_add_view(KDTextFeatureSpec "Text feature parameters" KDTextFeatureSpec.dd KDTextFeatureSpec.dd.log -nomodel -noarrayview)
 generate_gui_add_view(KDConstructionDomain "Variable construction parameters" KDConstructionDomain.dd KDConstructionDomain.dd.log -nomodel -noarrayview)
 generate_gui_add_view(KDConstructionRule "Construction rule" KDConstructionRule.dd KDConstructionRule.dd.log -nomodel)

# cmake-format: on

# Add dependency to ensure that genere will be built before the gui generation
generate_gui_add_view_add_dependency(KWUserInterface)
