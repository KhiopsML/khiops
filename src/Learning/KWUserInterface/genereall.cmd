@echo off

if exist KWDatabaseTextFile.h goto ERR
if exist KWDatabaseTextFile.cpp goto ERR
if exist KWSTDatabaseTextFile.h goto ERR
if exist KWSTDatabaseTextFile.cpp goto ERR
if exist KWMTDatabaseTextFile.h goto ERR
if exist KWMTDatabaseTextFile.cpp goto ERR
if exist KWMTDatabaseMapping.h goto ERR
if exist KWMTDatabaseMapping.cpp goto ERR
if exist KWPreprocessingSpec.h goto ERR
if exist KWPreprocessingSpec.cpp goto ERR
if exist KWDiscretizerSpec.h goto ERR
if exist KWDiscretizerSpec.cpp goto ERR
if exist KWGrouperSpec.h goto ERR
if exist KWGrouperSpec.cpp goto ERR
if exist KWGrouperKDMODL.h goto ERR
if exist KWGrouperKDMODL.cpp goto ERR
if exist KWBenchmarkSpec.h goto ERR
if exist KWBenchmarkSpec.cpp goto ERR
if exist KWBenchmarkClassSpec.h goto ERR
if exist KWBenchmarkClassSpec.cpp goto ERR
if exist KWLearningBenchmark.h goto ERR
if exist KWLearningBenchmark.cpp goto ERR
if exist KDConstructionDomain.h goto ERR
if exist KDConstructionDomain.cpp goto ERR
if exist KDConstructionRule.h goto ERR
if exist KDConstructionRule.cpp goto ERR
if exist KWRecodingSpec.h goto ERR
if exist KWRecodingSpec.cpp goto ERR
if exist KWAttributeConstructionSpec.h goto ERR
if exist KWAttributeConstructionSpec.cpp goto ERR
if exist KWAttributePairsSpec.h goto ERR
if exist KWAttributePairsSpec.cpp goto ERR
if not exist ..\KWModeling\KWRecodingSpec.h goto ERR
if not exist ..\KWModeling\KWRecodingSpec.cpp goto ERR
if not exist ..\KWModeling\KWAttributeConstructionSpec.h goto ERR
if not exist ..\KWModeling\KWAttributeConstructionSpec.cpp goto ERR

genere -noarrayview KWDatabase "Database" KWDatabase.dd
del KWDatabase.h
del KWDatabase.cpp 

genere  -super KWDatabase -noarrayview KWSTDatabaseTextFile "Database" KWSTDatabaseTextFile.dd
del KWSTDatabaseTextFile.h
del KWSTDatabaseTextFile.cpp 

genere  -super KWDatabase -noarrayview KWMTDatabaseTextFile "Database" KWMTDatabaseTextFile.dd
del KWMTDatabaseTextFile.h
del KWMTDatabaseTextFile.cpp 

genere KWMTDatabaseMapping "Multi-table mapping" KWMTDatabaseMapping.dd
del KWMTDatabaseMapping.h
del KWMTDatabaseMapping.cpp 

genere -noarrayview KWClassManagement "Dictionary management" KWClassManagement.dd

genere KWClassSpec "Dictionary" KWClassSpec.dd

genere KWAttributeSpec "Variable" KWAttributeSpec.dd

genere KWAttributeName "Variable" KWAttributeName.dd
del KWAttributeName.h
del KWAttributeName.cpp 

genere KWAttributePairName "Variable pair" KWAttributePairName.dd
del KWAttributePairName.h
del KWAttributePairName.cpp 

genere KWEvaluatedPredictorSpec "Evaluated predictor" KWEvaluatedPredictorSpec.dd

genere KWPreprocessingSpec "Preprocessing parameters" KWPreprocessingSpec.dd
del KWPreprocessingSpec.h
del KWPreprocessingSpec.cpp

genere -noarrayview KWDiscretizerSpec "Discretization" KWDiscretizerSpec.dd
del KWDiscretizerSpec.h
del KWDiscretizerSpec.cpp

genere -noarrayview KWGrouperSpec "Value grouping" KWGrouperSpec.dd
del KWGrouperSpec.h
del KWGrouperSpec.cpp

genere -noarrayview KWDataGridOptimizerParameters "Data Grid optimization" KWDataGridOptimizerParameters.dd
del KWDataGridOptimizerParameters.h
del KWDataGridOptimizerParameters.cpp

genere KWBenchmarkSpec "Benchmark" KWBenchmarkSpec.dd
del KWBenchmarkSpec.h
del KWBenchmarkSpec.cpp

genere -noarrayview KWLearningBenchmark "Learning benchmark" KWLearningBenchmark.dd
del KWLearningBenchmark.h
del KWLearningBenchmark.cpp

genere -noarrayview KWBenchmarkClassSpec "Benchmark dictionary" KWBenchmarkClassSpec.dd
del KWBenchmarkClassSpec.h
del KWBenchmarkClassSpec.cpp


copy ..\KWModeling\KWRecodingSpec.h KWRecodingSpec.h
copy ..\KWModeling\KWRecodingSpec.cpp KWRecodingSpec.cpp
genere -noarrayview KWRecodingSpec "Recoding parameters" KWRecodingSpec.dd
copy  KWRecodingSpec.h ..\KWModeling\KWRecodingSpec.h
copy  KWRecodingSpec.cpp ..\KWModeling\KWRecodingSpec.cpp
del KWRecodingSpec.h
del KWRecodingSpec.cpp

genere -noarrayview KWAttributePairsSpec "Variable pairs parameters" KWAttributePairsSpec.dd
del KWAttributePairsSpec.h
del KWAttributePairsSpec.cpp

genere -noarrayview KWAttributeConstructionSpec "Feature engineering parameters" KWAttributeConstructionSpec.dd
del KWAttributeConstructionSpec.h
del KWAttributeConstructionSpec.cpp


genere -noarrayview KDConstructionDomain "Variable construction parameters" KDConstructionDomain.dd
del KDConstructionDomain.h
del KDConstructionDomain.cpp
genere KDConstructionRule "Construction rule" KDConstructionRule.dd
del KDConstructionRule.h
del KDConstructionRule.cpp


goto END

:ERR
echo Erreur: une classe existe a tort (seule sa vue devrait etre a generer)
goto END

:END