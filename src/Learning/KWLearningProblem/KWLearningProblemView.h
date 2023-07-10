// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"

#include "KWLearningProblem.h"

#include "KWClassManagementView.h"
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
#include "LMLicenseManager.h"
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

#ifdef DEPRECATED_V10
	// DEPRECATED V10: ergonomie, conservee de facon cachee en V10 pour compatibilite ascendante des scenarios
	// Vue et donnees sur une base de test cachee correspondant a l'ancien onglet "Test database"
	// Les anciens scenarios peuvent toujours exploiter cette vue sans planter, mais cette vue est utilisee,
	// cela sera detecte au moment d'un mise a jour des donnees par l'interface. On recopiera alors
	// les donnees de la base de test obsolete vers la base de test, en emettant un warning pour l'utilisateur
	// (au plus une fois)
	KWDatabase* deprecatedEmptyDatabase;
	KWDatabase* deprecatedTestDatabase;
	KWDatabaseView* deprecatedTestDatabaseView;
	boolean bDeprecatedTestDataViewUsed;
	boolean bDeprecatedTestDataViewWarningEmited;
#endif // DEPRECATED_V10
};
