// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"

#include "CCLearningProblem.h"

#include "KWClassManagementView.h"
#include "KWDatabaseView.h"
#include "CCAnalysisSpecView.h"
#include "CCAnalysisResultsView.h"
#include "CCLearningProblemActionView.h"
#include "KWClassBuilderView.h"
#include "KWClassAttributeHelpList.h"
#include "KWDatabaseAttributeValuesHelpList.h"
#include "LMLicenseManager.h"
#include "KWLearningProblemHelpCard.h"

////////////////////////////////////////////////////////////
// Classe CCLearningProblemView
// Editeur de CCLearningProblem
class CCLearningProblemView : public UIObjectView
{
public:
	// Constructeur
	CCLearningProblemView();
	~CCLearningProblemView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Actions disponibles
	void BuildCoclustering();

	// Acces au probleme d'apprentissage
	void SetObject(Object* object) override;
	CCLearningProblem* GetLearningProblem();

	/////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Rafraichissement des listes d'aide
	void RefreshHelpLists();

	// Gestion des listes d'aide
	KWClassAttributeHelpList simpleAttributeHelpList;
	KWClassAttributeHelpList continuousAttributeHelpList;
};
