# On utilise les exceptions seulement pour le fichier KWLearningProject.cpp
if(NOT MSVC)
  set_source_files_properties(KWLearningProject.cpp PROPERTIES COMPILE_FLAGS -fexceptions)
endif(NOT MSVC)

file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KWLearningProblem STATIC "${cppfiles}")
set_khiops_options(KWLearningProblem)

target_include_directories(KWLearningProblem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KWLearningProblem PUBLIC KDDomainKnowledge DTForest KWDRRuleLibrary KWDataPreparation
                                               KWUserInterface SNBPredictor)
