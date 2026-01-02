// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KNITransfer.h"

// Parametrage des variables d'environnement pour les jeux de test de LearningTest (via .vs/launch.vs.json sous Windows Visual C++)
// KHIOPS_API_MODE="true"
// - pour que les chemins relatifs soient traites par rapport au cwd (current working directory) pour le stockage des rapports
// - sinon, en mode GUI, les chemins relatifs sont geres par rapport au repertoire contenant le fichier de donnees
// KhiopsFastExitMode="false"
// - pour que l'on puisse exploiter un scenario de test integralement, meme en cas d'erreurs multiples
// - sinon, on sort du scenario apres la premiere erreur

int main(int argc, char** argv)
{
	KNITransferProject learningProject;

	// Point d'arret sur l'allocation d'un bloc memoire
	// MemSetAllocIndexExit(1290133);

	// Lancement du projet
	learningProject.Start(argc, argv);
	return 0;
}
