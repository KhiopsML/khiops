@echo off

echo. > genere.log

genere -noarrayview KWAnalysisSpec "Parameters" KWAnalysisSpec.dd >> genere.log

genere -noarrayview KWRecoderSpec "Recoders" KWRecoderSpec.dd >> genere.log

genere -noarrayview KWAnalysisResults "Results" KWAnalysisResults.dd >> genere.log
