// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "SampleOneLearningProblem.h"
#include "KWLearningProblemView.h"

////////////////////////////////////////////////////////////
// Classe SampleOneLearningProblemView
//    Khiops: preparation des donnees
// Editeur de SampleOneLearningProblemView
class SampleOneLearningProblemView : public KWLearningProblemView
{
public:
	// Constructeur
	SampleOneLearningProblemView();
	~SampleOneLearningProblemView();

	// Actions etendues
	// Verification de la validite des specifications
	void CheckLearningSpec();

	// Affichage des attributs de la classe
	void ShowClass();

	// Affichage des valeurs des attribut d'un objet
	void ShowObject();

	// Ecriture d'un rapport de statistiques descriptives
	void ExportStats();

	// Acces au probleme d'apprentissage
	void SetObject(Object* object);
	SampleOneLearningProblem* GetSampleOneLearningProblem();
};

////////////////////////////////////////////////////////////
// Classe SampleOneAnalysisSpecView
//    Version specialisee des Analysis parameters permettant
//     de preciser les parametre de modelisation
// Editeur de SampleOneAnalysisSpec
class SampleOneAnalysisSpecView : public KWAnalysisSpecView
{
public:
	// Constructeur
	SampleOneAnalysisSpecView();
	~SampleOneAnalysisSpecView();
};
