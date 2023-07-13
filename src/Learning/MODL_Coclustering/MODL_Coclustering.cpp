// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MODL_Coclustering.h"

int main(int argc, char** argv)
{
	CCLearningProject learningProject;

	// Debogage sous Windows Visual C++ 2022 (bug https://github.com/microsoft/vscode-cpptools/issues/8084)
	// Choix en dur du repertoire de lancement (a commenter apres fin du debug)
	// ALString sUserRootPath = "D:/Users/miib6422/Documents/boullema/LearningTest/TestCoclustering/";
	// _chdir(sUserRootPath + "y_CoclusteringIV_Standard/IrisLight");

	// MemSetAllocIndexExit(1290133);

	// Activation de la gestion des signaux via des erreurs, pour afficher des messages d'erreur explicites
	// A potentiellement commnter sur certian IDE lors des phases de debuggage
	Global::ActivateSignalErrorManagement();

	// Lancement du projet
	learningProject.Start(argc, argv);

	// On renvoie 0 si tout s'est bien passe, 1 en cas de FatalError (dans Standard.cpp) et 2 si il y eu au moins
	// une erreur
	if (GetProcessId() == 0 and Global::IsAtLeastOneError())
		return EXIT_FAILURE + 1;
	else
		return EXIT_SUCCESS;
}
