@echo off

echo. > genere.log

call ..\..\genere -noarrayview CCVarPartCoclusteringSpec "Instances Variables coclustering parameters" CCVarPartCoclusteringSpec.dd >> genere.log

call ..\..\genere -noarrayview CCCoclusteringSpec "Coclustering parameters" CCCoclusteringSpec.dd >> genere.log

call ..\..\genere -noarrayview CCPostProcessingSpec "Simplification parameters" CCPostProcessingSpec.dd >> genere.log

call ..\..\genere CCPostProcessedAttribute "Coclustering variable" CCPostProcessedAttribute.dd >> genere.log

call ..\..\genere -noarrayview CCDeploymentSpec "Deployment parameters" CCDeploymentSpec.dd >> genere.log

call ..\..\genere -noarrayview CCAnalysisResults "Results" CCAnalysisResults.dd >> genere.log
