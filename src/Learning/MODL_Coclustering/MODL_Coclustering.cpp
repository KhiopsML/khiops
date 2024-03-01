// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MODL_Coclustering.h"

// Debogage sous Windows Visual C++ 2022 (bug https://github.com/microsoft/vscode-cpptools/issues/8084)
// Choix en dur du repertoire de lancement
void SetWindowsDebugDir(const ALString& sDatasetFamily, const ALString& sDataset)
{
#ifdef _WIN32
	ALString sUserRootPath;
	int nRet;

	// A parametrer pour chaque utilisateur
	// Devra etre fait plus proprement quand tout l'equipe sera sur git, par exemple via une variable
	// d'environnement et quelques commentaires clairs
	sUserRootPath = "D:/Users/miib6422/Documents/boullema/LearningTest/TestCoclustering/";

	// Pour permettre de continuer a utiliser LearningTest, on ne fait rien s'il y a deja un fichier test.prm
	// dans le repertoire courante
	if (FileService::FileExists("test.prm"))
		return;

	// Changement de repertoire, uniquement pour Windows
	nRet = _chdir(sUserRootPath + sDatasetFamily + "/" + sDataset);
#endif
}

int main(int argc, char** argv)
{
	CCLearningProject learningProject;

	// Activation de la gestion des signaux via des erreurs, pour afficher des messages d'erreur explicites
	// A potentiellement commenter sur certains IDE lors des phases de debuggage
	Global::ActivateSignalErrorManagement();

	// Choix du repertoire de lancement pour le debugage sous Windows (a commenter apres fin du debug)
	SetWindowsDebugDir("y_CoclusteringIV_Standard", "IrisLight");
	//SetWindowsDebugDir("y_CoclusteringIV_Standard", "Iris");
	//SetWindowsDebugDir("Standard", "Iris");

	// Point d'arret sur l'allocation d'un bloc memoire
	// MemSetAllocIndexExit(30406);

	// Choix du repertoire de lancement pour le debugage sous Windows (a commenter apres fin du debug)
	// SetWindowsDebugDir("Standard", "Iris");

	// Lancement du projet
	learningProject.Start(argc, argv);

	// On renvoie 0 si tout s'est bien passe, 1 en cas de FatalError (dans Standard.cpp) et 2 si il y eu au moins une erreur
	if (GetProcessId() == 0 and Global::IsAtLeastOneError())
		return EXIT_FAILURE + 1;
	else
		return EXIT_SUCCESS;
}
