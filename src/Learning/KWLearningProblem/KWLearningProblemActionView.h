// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWLearningProblemView;

#include "UserInterface.h"
#include "KWLearningProblem.h"
#include "KWLearningProblemView.h"
#include "KWDatabaseView.h"
#include "KWDatabaseTransferView.h"

////////////////////////////////////////////////////////////
// Classe KWLearningProblemActionView
//    Actions d'analyse deportees de KWLearningProblemView
class KWLearningProblemActionView : public UIObjectView
{
public:
	// Constructeur
	KWLearningProblemActionView();
	~KWLearningProblemActionView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Actions de menu
	void CheckData();
	void SortDataTableByKey();
	void ExtractKeysFromDataTable();
	void BuildConstructedDictionary();
	void ComputeStats();
	void TransferDatabase();
	void EvaluatePredictors();
	void InterpretPredictor();
	void ReinforcePredictor();

	// Acces au probleme d'apprentissage
	KWLearningProblem* GetLearningProblem();

	// Acces a la vue principale sur le probleme d'apprentissage
	KWLearningProblemView* GetLearningProblemView();
};
