@echo off

echo. > genere.log

genere -noarrayview CCInstancesVariablesCoclusteringSpec ""Instances Variables Coclustering parameters" CCInstancesVariablesCoclusteringSpec.dd >> genere.log

genere -noarrayview CCCoclusteringSpec "Coclustering parameters" CCCoclusteringSpec.dd >> genere.log

genere -noarrayview CCPostProcessingSpec "Simplification parameters" CCPostProcessingSpec.dd >> genere.log

genere CCPostProcessedAttribute "Coclustering variable" CCPostProcessedAttribute.dd >> genere.log

genere -noarrayview CCDeploymentSpec "Deployment parameters" CCDeploymentSpec.dd >> genere.log

genere -noarrayview CCAnalysisResults "Results" CCAnalysisResults.dd >> genere.log
