@echo off

echo. > genere.log

rem On ne genere que la partie modele, la partie vue etant uniquement composee de deux sous-fiches
genere -noview -noarrayview KWModelingSpec "Predictors" KWModelingSpec.dd >> genere.log

rem On genere la partie vue uniquement pour deux vues distinct sur le modele
genere -nomodel -noarrayview -specificmodel KWModelingSpec KWModelingAdvancedSpec "Advanced predictor parameters" KWModelingAdvancedSpec.dd >> genere.log
genere -nomodel -noarrayview -specificmodel KWModelingSpec KWModelingExpertSpec "Expert predictor parameters" KWModelingExpertSpec.dd >> genere.log

genere -noarrayview KWAnalysisSpec "Parameters" KWAnalysisSpec.dd >> genere.log

genere -noarrayview KWRecoderSpec "Recoders" KWRecoderSpec.dd >> genere.log

rem remarque: tous les champs de choix de noms de fichiers sont exclus de la vue (Invisible à true dans le fichier .dd)
genere -noarrayview KWAnalysisResults "Results" KWAnalysisResults.dd >> genere.log
