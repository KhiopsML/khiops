@echo off

echo. > genere.log

rem On ne call ..\..\genere que la partie modele, la partie vue etant uniquement composee de deux sous-fiches
call ..\..\genere -noview -noarrayview KWModelingSpec "Predictors" KWModelingSpec.dd >> genere.log

rem On call ..\..\genere la partie vue uniquement pour deux vues distinct sur le modele
call ..\..\genere -nomodel -noarrayview -specificmodel KWModelingSpec KWModelingAdvancedSpec "Advanced predictor parameters" KWModelingAdvancedSpec.dd >> genere.log
call ..\..\genere -nomodel -noarrayview -specificmodel KWModelingSpec KWModelingExpertSpec "Expert predictor parameters" KWModelingExpertSpec.dd >> genere.log

call ..\..\genere -noarrayview KWAnalysisSpec "Parameters" KWAnalysisSpec.dd >> genere.log

call ..\..\genere -noarrayview KWRecoderSpec "Recoders" KWRecoderSpec.dd >> genere.log

rem remarque: tous les champs de choix de noms de fichiers sont exclus de la vue (Invisible à true dans le fichier .dd)
call ..\..\genere -noarrayview KWAnalysisResults "Results" KWAnalysisResults.dd >> genere.log
