// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "CMLearningProblem.h"
#include "KWLearningProblemView.h"
#include "CMModelingSpecView.h"

////////////////////////////////////////////////////////////////
/// Classe CMLearningProblemView : Vue sur la gestion de l'apprentissage avec Classifieur Majoritaire
class CMLearningProblemView : public KWLearningProblemView
{
public:
	// Constructeur
	CMLearningProblemView();
	~CMLearningProblemView();

	// Actions etendues
	// Rien pour l'instant

	// Acces au probleme d'apprentissage
	// void SetObject(Object* object);
	CMLearningProblem* GetCMLearningProblem();
};

////////////////////////////////////////////////////////////
// Classe CMAnalysisSpecView
//    Version specialisee des Analysis parameters permettant
//     de preciser les parametre de modelisation
// Editeur de CMAnalysisSpec
class CMAnalysisSpecView : public KWAnalysisSpecView
{
public:
	// Constructeur
	CMAnalysisSpecView();
	~CMAnalysisSpecView();
};