@echo off

echo. > genere.log

genere -noarrayview CCVarPartCoclusteringSpec "Instances Variables coclustering parameters" CCVarPartCoclusteringSpec.dd >> genere.log

genere -noarrayview CCCoclusteringSpec "Coclustering parameters" CCCoclusteringSpec.dd >> genere.log

genere -noarrayview CCPostProcessingSpec "Simplification parameters" CCPostProcessingSpec.dd >> genere.log

genere CCPostProcessedAttribute "Coclustering variable" CCPostProcessedAttribute.dd >> genere.log

genere -noarrayview CCDeploymentSpec "Deployment parameters" CCDeploymentSpec.dd >> genere.log

genere -noarrayview CCAnalysisResults "Results" CCAnalysisResults.dd >> genere.log
