@echo off

echo. > genere.log

rem Dans la plus cas, on ne call ..\..\genere que les classe de type vue, les classe de type modele etant geree par ailleur

call ..\..\genere -nomodel -noarrayview KWDatabase "Database" KWDatabase.dd >> genere.log

call ..\..\genere -nomodel -noarrayview -specificmodel KWDatabase KWDatabaseSampling "Sampling" KWDatabaseSampling.dd >> genere.log

call ..\..\genere -nomodel -noarrayview -specificmodel KWDatabase  KWDatabaseSelection "Selection" KWDatabaseSelection.dd >> genere.log

call ..\..\genere -nomodel -noarrayview -specificmodel KWSTDatabaseTextFile  KWSTDatabaseTextFileData "Data" KWSTDatabaseTextFileData.dd >> genere.log

call ..\..\genere -nomodel -noarrayview -specificmodel KWMTDatabaseTextFile  KWMTDatabaseTextFileData "Data" KWMTDatabaseTextFileData.dd >> genere.log

call ..\..\genere -nomodel KWMTDatabaseMapping "Multi-table mapping" KWMTDatabaseMapping.dd >> genere.log

call ..\..\genere KWClassSpec "Dictionary" KWClassSpec.dd >> genere.log

call ..\..\genere KWAttributeSpec "Variable" KWAttributeSpec.dd >> genere.log

call ..\..\genere -nomodel KWAttributeName "Variable" KWAttributeName.dd >> genere.log

call ..\..\genere -nomodel KWAttributePairName "Variable pair" KWAttributePairName.dd >> genere.log

call ..\..\genere KWEvaluatedPredictorSpec "Evaluated predictor" KWEvaluatedPredictorSpec.dd >> genere.log

call ..\..\genere -nomodel -noarrayview KWPreprocessingSpec "Preprocessing parameters" KWPreprocessingSpec.dd >> genere.log

call ..\..\genere -nomodel -noarrayview -specificmodel KWPreprocessingSpec KWPreprocessingAdvancedSpec "Unsupervised parameters" KWPreprocessingAdvancedSpec.dd >> genere.log

call ..\..\genere -nomodel -noarrayview KWDataGridOptimizerParameters "Data Grid optimization" KWDataGridOptimizerParameters.dd >> genere.log

call ..\..\genere -nomodel KWBenchmarkSpec "Benchmark" KWBenchmarkSpec.dd >> genere.log

call ..\..\genere -nomodel -noarrayview KWLearningBenchmark "Learning benchmark" KWLearningBenchmark.dd >> genere.log

call ..\..\genere -nomodel -noarrayview KWBenchmarkClassSpec "Benchmark dictionary" KWBenchmarkClassSpec.dd >> genere.log

rem La partie modele est geree dans la librairie KWModeling
call ..\..\genere -outputdir ..\KWModeling -noview KWRecodingSpec "Recoding parameters" KWRecodingSpec.dd >> genere.log
call ..\..\genere -nomodel -noarrayview KWRecodingSpec "Recoding parameters" KWRecodingSpec.dd >> genere.log

call ..\..\genere -nomodel -noarrayview KWAttributePairsSpec "Variable pairs parameters" KWAttributePairsSpec.dd >> genere.log

call ..\..\genere -nomodel -noarrayview KWAttributeConstructionSpec "Feature engineering parameters" KWAttributeConstructionSpec.dd >> genere.log

rem La partie modele est geree dans la librairie KDDomainKnowledge
call ..\..\genere -outputdir ..\KDDomainKnowledge -noview KDTextFeatureSpec "Text feature parameters" KDTextFeatureSpec.dd >> genere.log
call ..\..\genere -nomodel -noarrayview KDTextFeatureSpec "Text feature parameters" KDTextFeatureSpec.dd >> genere.log

call ..\..\genere -nomodel -noarrayview KDConstructionDomain "Variable construction parameters" KDConstructionDomain.dd >> genere.log

call ..\..\genere -nomodel KDConstructionRule "Construction rule" KDConstructionRule.dd >> genere.log
