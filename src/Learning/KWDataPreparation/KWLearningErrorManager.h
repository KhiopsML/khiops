// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Ermgt.h"
#include "Object.h"
#include "JSONFile.h"

//////////////////////////////////////////////////////////////////
// Classe KWLearningErrorManager
// Gestion des erreurs pour un probleme d'apprentissage
//
// Cette classe permet d'intercepter tous les message d'erreur emises
// lors de l'apprentissage et d'en archiver une sous-partie, qui pourra
// etre utilisee pour l'ecriture de rapport
class KWLearningErrorManager : public Object
{
public:
	// Demarrage de la collecte des erreurs
	// Seuls les warnings et les erreurs sont collectes
	static void BeginErrorCollection();

	// Arret de gestion des erreurs
	// Les erreurs collectees sont detruites
	static void EndErrorCollection();

	// Ajout d'une titre de taches permettant de structurer les erreurs en sous-parties,
	// comme par exemple "Data preparation", "modeling"...
	// Il est necessaire d'ajouter un titre des le depart
	static void AddTask(const ALString& sTitle);

	// Ecriture de l'ensemble des erreurs collectees dans un fichier json sous la forme d'un tableau
	static void WriteJSONKeyReport(JSONFile* fJSON);

	///////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Fonction dediee a la collecte des erreurs
	static void CollectErrorFunction(const Error* e);

	// Test si une erreur est un titre de section
	static boolean IsTask(Error* error);

	// Fonction de gestion des erreurs courante
	static DisplayErrorFunction currentDisplayErrorFunction;

	// Nombre de taches
	static int nTaskNumber;

	// Tableau des erreur collectees
	static ObjectArray oaCollectedErrors;
};
