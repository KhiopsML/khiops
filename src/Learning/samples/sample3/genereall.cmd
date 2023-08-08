@echo off

echo. > genere.log

call ..\..\..\genere -super KWModelingSpec -noarrayview MYModelingSpec "Modeling parameters" MYModelingSpec.dd >> genere.log

call ..\..\..\genere -super KWAnalysisResults -noarrayview MYAnalysisResults "Analysis results" MYAnalysisResults.dd  >> genere.log
