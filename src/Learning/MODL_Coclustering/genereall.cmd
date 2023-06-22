@echo off

genere -noarrayview CCAnalysisSpec "Parameters" CCAnalysisSpec.dd

genere -noarrayview CCCoclusteringSpec "Coclustering parameters" CCCoclusteringSpec.dd

genere -noarrayview CCPostProcessingSpec "Simplification parameters" CCPostProcessingSpec.dd
genere CCPostProcessedAttribute "Coclustering variable" CCPostProcessedAttribute.dd

genere -noarrayview CCDeploymentSpec "Deployment parameters" CCDeploymentSpec.dd

genere -noarrayview CCAnalysisResults "Results" CCAnalysisResults.dd



