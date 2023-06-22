Exemple avance d'utilisation des composants de la librairie LearningEnv, en mode multi-tables == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == =

																													    Fonctionnalites : Interface similaire a l'outil Khiops Predicteurs Naive Bayes et Selective Naive Bayes
																																  En mode multi -
																															      table,
																												     acces aux fonctionnalites multi - tables - edition d'un mapping multi-table - preparation multi - tables des attributs secondaires.si option "Compute basic secondary stats" activee dans le panneau "Analysis results"->creation automatique d'agregats élémentaires dans le cas multi-tables .table en lien 0 - 1 : GetValue et GetValueC.table en lien 0 - n : TableMean et TableMode - apprentissage et evaluation multi - tables En mode expert(set LearningExpertMode = true), possibilite de lancer des benchmarks

																																																																																							      Fichiers sources : genereall.cmd Commande de génération de code à partir des fichiers.dd

																																																																																										 MYModelingSpec.dd Paramétrage de la génération des fichiers suivants MYModelingSpec.h Specialisation de KWModelingSpec pour parametrage des predicteurs NB et SNB MYModelingSpec.cpp MYModelingSpecView.h MYModelingSpecView.cpp

																																																																																										 MYAnalysisResults.dd Paramétrage de l'option "Compute basic secondary stats" MYAnalysisResults.h Specialisation de KWAnalysisResults pour parametrage de l'optiopn "Compute basic secondary stats" MYAnalysisResults.cpp MYAnalysisResultsView.h MYAnalysisResultsView.cpp

																																																																																										 MYLearningProblem.h Specialisation de KWLearningProblem pour integrer fonctionnalite nouvelles MYLearningProblem.cpp MYLearningProblemView.h MYLearningProblemView.cpp

																																																																																										 MYLearningProject.h Specialisation de KWLearningProject pour integrer fonctionnalite nouvelles MYLearningProject.cpp

																																																																																										 MYMain.h Main de lancement MYMain.cpp

																																																																																										 Jeu d'essai multi-table dans le répertoire Data: voir Data/readme.txt

																																																																																										 Installation : Creer un projet utilisant LearningEnv et Norm Compiler les fichiers sources de l'exemple

																																																																																												Utilisation : voir Data
																																																																																							      / readme.txt
