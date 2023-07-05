// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWLearningProject.h"
#include "CMLearningProblem.h"
#include "CMLearningProblemView.h"

// Service de lancement du projet Classifieur majoritaire
class CMLearningProject : public KWLearningProject
{
public:
	// Constructeur
	CMLearningProject();
	~CMLearningProject();

	///////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	KWLearningProblem* CreateLearningProblem();
	KWLearningProblemView* CreateLearningProblemView();

	// Initialisation de l'environnement d'apprentissage (enregistrement des regles, predicteurs...)
	// Enrichissement de la methode KwLearningProject par specifites arbres de decision
	void OpenLearningEnvironnement();
};