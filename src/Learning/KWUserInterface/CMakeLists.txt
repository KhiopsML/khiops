file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KWUserInterface STATIC "${cppfiles}")
set_khiops_options(KWUserInterface)
# Dans la plus cas, on ne genere que les classe de type vue, les classe de type modele etant geree par ailleur
# cmake-format: off
add_view_generation(KWUserInterface KWDatabase "Database" KWDatabase.dd KWDatabase.dd.log -nomodel -noarrayview)
add_view_generation(KWUserInterface KWDatabaseSampling "Sampling" KWDatabaseSampling.dd KWDatabaseSampling.dd.log -nomodel -noarrayview -specificmodel KWDatabase)
add_view_generation(KWUserInterface KWDatabaseSelection "Selection" KWDatabaseSelection.dd KWDatabaseSelection.dd.log -nomodel -noarrayview -specificmodel KWDatabase)
add_view_generation(KWUserInterface KWSTDatabaseTextFileData "Data" KWSTDatabaseTextFileData.dd KWSTDatabaseTextFileData.dd.log -nomodel -noarrayview -specificmodel KWSTDatabaseTextFile)
add_view_generation(KWUserInterface KWMTDatabaseTextFileData "Data" KWMTDatabaseTextFileData.dd KWMTDatabaseTextFileData.dd.log -nomodel -noarrayview -specificmodel KWMTDatabaseTextFile)
add_view_generation(KWUserInterface KWMTDatabaseMapping "Multi-table mapping" KWMTDatabaseMapping.dd KWMTDatabaseMapping.dd.log -nomodel)
add_view_generation(KWUserInterface KWClassSpec "Dictionary" KWClassSpec.dd KWClassSpec.dd.log)
add_view_generation(KWUserInterface KWAttributeSpec "Variable" KWAttributeSpec.dd KWAttributeSpec.dd.log)
add_view_generation(KWUserInterface KWAttributeName "Variable" KWAttributeName.dd KWAttributeName.dd.log -nomodel)
add_view_generation(KWUserInterface KWAttributePairName "Variable pair" KWAttributePairName.dd KWAttributePairName.dd.log -nomodel)
add_view_generation(KWUserInterface KWEvaluatedPredictorSpec "Evaluated predictor" KWEvaluatedPredictorSpec.dd KWEvaluatedPredictorSpec.dd.log)
add_view_generation(KWUserInterface KWPreprocessingSpec "Preprocessing parameters" KWPreprocessingSpec.dd KWPreprocessingSpec.dd.log -nomodel -noarrayview)
add_view_generation(KWUserInterface KWPreprocessingAdvancedSpec "Unsupervised parameters" KWPreprocessingAdvancedSpec.dd KWPreprocessingAdvancedSpec.dd.log -nomodel -noarrayview -specificmodel KWPreprocessingSpec)
add_view_generation(KWUserInterface KWDataGridOptimizerParameters "Data Grid optimization" KWDataGridOptimizerParameters.dd KWDataGridOptimizerParameters.dd.log -nomodel -noarrayview)
add_view_generation(KWUserInterface KWBenchmarkSpec "Benchmark" KWBenchmarkSpec.dd KWBenchmarkSpec.dd.log -nomodel)
add_view_generation(KWUserInterface KWLearningBenchmark "Learning benchmark" KWLearningBenchmark.dd KWLearningBenchmark.dd.log -nomodel -noarrayview)
add_view_generation(KWUserInterface KWBenchmarkClassSpec "Benchmark dictionary" KWBenchmarkClassSpec.dd KWBenchmarkClassSpec.dd.log -nomodel -noarrayview)

# La partie modele est geree dans la librairie KWModeling
add_view_generation(KWUserInterface KWRecodingSpec "Recoding parameters" KWRecodingSpec.dd KWRecodingSpec.dd.log -nomodel -noarrayview)
add_view_generation(KWUserInterface KWAttributePairsSpec "Variable pairs parameters" KWAttributePairsSpec.dd KWAttributePairsSpec.dd.log -nomodel -noarrayview)
add_view_generation(KWUserInterface KWAttributeConstructionSpec "Feature engineering parameters" KWAttributeConstructionSpec.dd KWAttributeConstructionSpec.dd.log -nomodel -noarrayview)
add_view_generation(KWUserInterface KWSelectionParameters "Selection parameters" ../KWModeling/KWSelectionParameters.dd KWSelectionParameters.dd.log  -nomodel -noarrayview)
add_view_generation(KWUserInterface KWTrainParameters "Train parameters" ../KWModeling/KWTrainParameters.dd KWTrainParameters.dd.log -nomodel -noarrayview)

# La partie modele est geree dans la librairie KDDomainKnowledge
add_view_generation(KWUserInterface KDTextFeatureSpec "Text feature parameters" KDTextFeatureSpec.dd KDTextFeatureSpec.dd.log -nomodel -noarrayview)
add_view_generation(KWUserInterface KDConstructionDomain "Variable construction parameters" KDConstructionDomain.dd KDConstructionDomain.dd.log -nomodel -noarrayview)
add_view_generation(KWUserInterface KDConstructionRule "Construction rule" KDConstructionRule.dd KDConstructionRule.dd.log -nomodel)
# cmake-format: on

target_include_directories(KWUserInterface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KWUserInterface PUBLIC KDDomainKnowledge KWDataPreparation KWModeling MHHistograms)
