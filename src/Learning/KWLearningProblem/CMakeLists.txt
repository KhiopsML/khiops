# On utilise les exceptions seulement pour le fichier KWLearningProject.cpp
if(NOT MSVC)
  set_source_files_properties(KWLearningProject.cpp PROPERTIES COMPILE_FLAGS -fexceptions)
endif(NOT MSVC)

file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KWLearningProblem STATIC "${cppfiles}")
set_khiops_options(KWLearningProblem)
# Generate cpp files from dd file
# cmake-format: off
# On ne genere que la partie modele, la partie vue etant uniquement composee de deux sous-fiches
add_view_generation(KWLearningProblem KWModelingSpec "Predictors" KWModelingSpec.dd KWModelingSpec.dd.log  -noview -noarrayview)

# On ne genere la partie vue uniquement pour deux vues distinct sur le modele
add_view_generation(KWLearningProblem KWModelingAdvancedSpec "Advanced predictor parameters" KWModelingAdvancedSpec.dd KWModelingAdvancedSpec.dd.log -nomodel -noarrayview -specificmodel KWModelingSpec)
add_view_generation(KWLearningProblem KWModelingExpertSpec "Expert predictor parameters" KWModelingExpertSpec.dd KWModelingExpertSpec.dd.log -nomodel -noarrayview -specificmodel KWModelingSpec)
add_view_generation(KWLearningProblem KWAnalysisSpec "Parameters" KWAnalysisSpec.dd KWAnalysisSpec.dd.log  -noarrayview)
add_view_generation(KWLearningProblem KWRecoderSpec "Recoders" KWRecoderSpec.dd KWRecoderSpec.dd.log  -noarrayview)

# remarque: tous les champs de choix de noms de fichiers sont exclus de la vue (Invisible a true dans le fichier .dd)
add_view_generation(KWLearningProblem KWAnalysisResults "Results" KWAnalysisResults.dd KWAnalysisResults.dd.log  -noarrayview)
# cmake-format: on
target_include_directories(KWLearningProblem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  KWLearningProblem
  PUBLIC KDDomainKnowledge
         DTForest
         KIInterpretation
         KWDRRuleLibrary
         KWDataPreparation
         KWUserInterface
         SNBPredictor
         MHHistograms)
