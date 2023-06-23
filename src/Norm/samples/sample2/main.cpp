// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "main.h"

// Routine principale de lancement d'un projet avec interface
// Edition d'une vue sur l'objet projet (ici reduit a saplus simple expression)
void StartSample2()
{
	PRWorker worker;
	PRWorkerView workerView;

	// Attachement de l'objet edite a sa vue
	workerView.SetObject(&worker);

	// Ouverture de la fentere principale
	workerView.Open();
}

int main(int argv, char** argc)
{
	// Routine a activer pour mettre un point d'arret sur l'allocateur memoire, en
	// indiquant le numero de block memoire posant probleme.
	// Il faut mettre un point d'arret dans le source assert\standard.cpp de Norm,
	// sur la ligne ou est appelee l'instruct exit().
	// MemSetAllocIndexExit(???);

	// Analyse de la ligne de commande
	// Permet d'enregistrer ou rejouer un scenario
	UIObject::ParseMainParameters(argv, argc);

	// Lancement de l'outil
	StartSample2();

	return 0;
}