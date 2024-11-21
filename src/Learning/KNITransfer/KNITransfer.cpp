// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KNITransfer.h"

// Debogage sous Windows Visual C++ 2022 (bug https://github.com/microsoft/vscode-cpptools/issues/8084)
// Choix en dur du repertoire de lancement
void SetWindowsDebugDir(const ALString& sDatasetFamily, const ALString& sDataset)
{
#ifdef _WIN32
	ALString sUserRootPath;
	int nRet;

	// Parametrage du repertoire racine de LearningTest via une variable d'environnement "LearningTestDir"
	// Il est recommande de positionner cette variable d'environnement via le fichier "launch.vs.json"
	// du repertoire ".vs", avec la cle "env"
	// Cf. wiki https://github.com/KhiopsML/khiops/wiki/Setting-Up-the-Development-Environment
	sUserRootPath = p_getenv("LearningTestDir");
	sUserRootPath += "/TestKNI/";

	// Pour permettre de continuer a utiliser LearningTest, on ne fait rien s'il y a deja un fichier test.prm
	// dans le repertoire courant
	if (FileService::FileExists("test.prm"))
		return;

	// Changement de repertoire, uniquement pour Windows
	nRet = _chdir(sUserRootPath + sDatasetFamily + "/" + sDataset);
	assert(nRet == 0);
#endif
}

int main(int argc, char** argv)
{
	KNITransferProject learningProject;

	// MemSetAllocIndexExit(1290133);

	// Choix du repertoire de lancement pour le debugage sous Windows (a commenter apres fin du debug)
	// SetWindowsDebugDir("Standard", "Iris");

	// Lancement du projet
	learningProject.Start(argc, argv);
	return 0;
}
