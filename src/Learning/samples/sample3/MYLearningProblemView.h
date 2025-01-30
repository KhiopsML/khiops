// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "MYLearningProblem.h"
#include "KWLearningProblemView.h"
#include "KWLearningBenchmarkView.h"
#include "KWLearningBenchmarkView.h"
#include "KWAnalysisSpecView.h"
#include "MYModelingSpecView.h"
#include "MYAnalysisResultsView.h"

////////////////////////////////////////////////////////////
// Classe MYLearningProblemView
//    Khiops: preparation des donnees
// Editeur de MYLearningProblemView
class MYLearningProblemView : public KWLearningProblemView
{
public:
	// Constructeur
	MYLearningProblemView();
	~MYLearningProblemView();

	// Actions etendues
	void ClassifierBenchmark();
	void RegressorBenchmark();

	// Acces au probleme d'apprentissage
	void SetObject(Object* object);
	MYLearningProblem* GetMyLearningProblem();
};

////////////////////////////////////////////////////////////
// Classe MYLearningProblemExtendedActionView
//    Actions d'analyse etendues deportees de MYLearningProblemView
//    Fiche ne contenant que des actions (pour un menu supplementaire)
//     sans maquettage de champ a l'interface
class MYLearningProblemExtendedActionView : public UIObjectView
{
public:
	// Constructeur
	MYLearningProblemExtendedActionView();
	~MYLearningProblemExtendedActionView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object);

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object);

	// Actions de menu
	void ClassifierBenchmark();
	void RegressorBenchmark();

	// Acces au probleme d'apprentissage
	MYLearningProblem* GetMyLearningProblem();

	// Acces a la vue principale sur le probleme d'apprentissage
	MYLearningProblemView* GetMyLearningProblemView();
};

////////////////////////////////////////////////////////////
// Classe MYAnalysisSpecView
//    Version specialisee des Analysis parameters permettant
//     de preciser les parametre de modelisation
// Editeur de MYAnalysisSpec
class MYAnalysisSpecView : public KWAnalysisSpecView
{
public:
	// Constructeur
	MYAnalysisSpecView();
	~MYAnalysisSpecView();
};
