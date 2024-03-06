# On utilise les exceptions seulement pour le fichier KWLearningProject.cpp
if(NOT MSVC)
  set_source_files_properties(KWLearningProject.cpp PROPERTIES COMPILE_FLAGS -fexceptions)
endif(NOT MSVC)

file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
generate_gui_add_view_get_log_list(ddlogfiles)

add_library(KWLearningProblem STATIC "${cppfiles}" "${ddlogfiles}")
set_khiops_options(KWLearningProblem)

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

# cmake-format: off

# Generate cpp files from dd file

# On ne genere que la partie modele, la partie vue etant uniquement composee de deux sous-fiches
 generate_gui_add_view(KWModelingSpec "Predictors" KWModelingSpec.dd KWModelingSpec.dd.log  -noview -noarrayview)
         
# On ne genere la partie vue uniquement pour deux vues distinct sur le modele
 generate_gui_add_view(KWModelingAdvancedSpec "Advanced predictor parameters" KWModelingAdvancedSpec.dd KWModelingAdvancedSpec.dd.log -nomodel -noarrayview -specificmodel KWModelingSpec)
 generate_gui_add_view(KWModelingExpertSpec "Expert predictor parameters" KWModelingExpertSpec.dd KWModelingExpertSpec.dd.log -nomodel -noarrayview -specificmodel KWModelingSpec)
 generate_gui_add_view(KWAnalysisSpec "Parameters" KWAnalysisSpec.dd KWAnalysisSpec.dd.log  -noarrayview)     
 generate_gui_add_view(KWRecoderSpec "Recoders" KWRecoderSpec.dd KWRecoderSpec.dd.log  -noarrayview)

# remarque: tous les champs de choix de noms de fichiers sont exclus de la vue (Invisible a true dans le fichier .dd)
 generate_gui_add_view(KWAnalysisResults "Results" KWAnalysisResults.dd KWAnalysisResults.dd.log  -noarrayview)

# Add dependency to ensure that genere will be built before the gui generation
 generate_gui_add_view_add_dependency(KWLearningProblem)
# cmake-format: on
