// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"

#include "KWLearningProblem.h"

#include "KWClassManagementActionView.h"
#include "KWDatabaseView.h"
#include "KWPredictorEvaluatorView.h"
#include "KWDataGridOptimizerParametersView.h"
#include "KWAnalysisSpecView.h"
#include "KWAnalysisResultsView.h"
#include "KWLearningProblemActionView.h"
#include "KWDataTableSorterView.h"
#include "KWDataTableKeyExtractorView.h"
#include "KWClassAttributeHelpList.h"
#include "KWDatabaseAttributeValuesHelpList.h"
#include "KWLearningProblemHelpCard.h"

class KWLearningProblemActionView;

////////////////////////////////////////////////////////////
// Classe KWLearningProblemView
// Editeur de KWLearningProblem
class KWLearningProblemView : public UIObjectView
{
public:
	// Constructeur
	// La fenetre principale a pour titre GetLearningMainWindowTitle()
	KWLearningProblemView();
	~KWLearningProblemView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Actions disponibles
	void CheckData();
	void SortDataTableByKey();
	void ExtractKeysFromDataTable();
	void BuildConstructedDictionary();
	void ComputeStats();
	void TransferDatabase();
	void EvaluatePredictors();
	void InterpretPredictor();

	// Acces au probleme d'apprentissage
	void SetObject(Object* object) override;
	KWLearningProblem* GetLearningProblem();

	/////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Construction de dictionnaire a partir d'un fichier
	// On fait une detection automatique de format suivi d'une construction de dictionnaire,
	// pour permettre l'analyse de donnees sans dictionnaire prealable
	// On renvoie true OK
	// Sinon, un nouveau dictionnaire de nom base sur le nom du fichier, est cree est disponible en memoire
	// Il n'est pas sauve dansd le fichier des dictionnaires
	boolean BuildClassFromDataTable();

	// Rafraichissement des listes d'aide
	void RefreshHelpLists();

	// Gestion des listes d'aide
	KWClassAttributeHelpList classAttributeHelpList;
	KWDatabaseAttributeValuesHelpList dbTargetValuesHelpList;

	// Memorisation des vues sur les base de train et de test
	// Ces vues sont parametrees dans le constructeur, pour ce qui concerne les liste d'aide principalement
	// La vue sur la base de train fait partie de l'interface du LearningProblem, alors que la vue sur
	// la base de test sert a parametrer le bouton "InspectTestDatabaseSettings" de la vue sur la base de train,
	// ce qui correspond a la nouvelle ergonomie depuis Khiops V10
	KWDatabaseView* trainDatabaseView;
	KWDatabaseView* testDatabaseView;
};
