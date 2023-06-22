@echo off

echo. > genere.log

genere -super KWModelingSpec -noarrayview MYModelingSpec "Modeling parameters" MYModelingSpec.dd >> genere.log

genere -super KWAnalysisResults -noarrayview MYAnalysisResults "Analysis results" MYAnalysisResults.dd  >> genere.log
