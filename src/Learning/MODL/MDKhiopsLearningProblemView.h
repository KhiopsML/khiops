// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "MDKhiopsLearningProblem.h"
#include "KWLearningProblemView.h"
#include "KWLearningBenchmarkView.h"
#include "KWAnalysisSpecView.h"
#include "MDKhiopsModelingSpecView.h"
#include "LMLicenseManager.h"

////////////////////////////////////////////////////////////
// Classe MDKhiopsLearningProblemView
//    Khiops: preparation des donnees
// Editeur de MDKhiopsLearningProblemView
class MDKhiopsLearningProblemView : public KWLearningProblemView
{
public:
	// Constructeur
	MDKhiopsLearningProblemView();
	~MDKhiopsLearningProblemView();

	// Actions etendues
	void ClassifierBenchmark();
	void RegressorBenchmark();
	void ClassifierBenchmarkUnivariate();
	void ClassifierBenchmarkBivariate();

	// Acces au probleme d'apprentissage
	void SetObject(Object* object) override;
	MDKhiopsLearningProblem* GetKhiopsLearningProblem();
};

////////////////////////////////////////////////////////////
// Classe MDKhiopsLearningProblemExtendedActionView
//    Actions d'analyse etendues deportees de MDKhiopsLearningProblemView
//    Fiche ne contenant que des actions (pour un menu supplementaire)
//     sans maquettage de champ a l'interface
class MDKhiopsLearningProblemExtendedActionView : public UIObjectView
{
public:
	// Constructeur
	MDKhiopsLearningProblemExtendedActionView();
	~MDKhiopsLearningProblemExtendedActionView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Actions de menu
	void ClassifierBenchmark();
	void RegressorBenchmark();
	void ClassifierBenchmarkUnivariate();
	void ClassifierBenchmarkBivariate();

	// Acces au probleme d'apprentissage
	MDKhiopsLearningProblem* GetKhiopsLearningProblem();

	// Acces a la vue principale sur le probleme d'apprentissage
	MDKhiopsLearningProblemView* GetKhiopsLearningProblemView();
};

////////////////////////////////////////////////////////////
// Classe MDKhiopsAnalysisSpecView
//    Version specialisee des Analysis specs permettant
//     de preciser les parametre de modelisation
// Editeur de MDKhiopsAnalysisSpec
class MDKhiopsAnalysisSpecView : public KWAnalysisSpecView
{
public:
	// Constructeur
	MDKhiopsAnalysisSpecView();
	~MDKhiopsAnalysisSpecView();
};
