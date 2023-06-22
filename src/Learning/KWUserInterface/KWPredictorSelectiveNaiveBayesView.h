// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWPredictorView.h"
#include "KWPredictorSelectiveNaiveBayes.h"
#include "KWSelectionParametersView.h"

////////////////////////////////////////////////////////////////////////////////
// Vue sur le parametrage specifique d'un classifieur Selective Naive Bayes
class KWPredictorSelectiveNaiveBayesView : public KWPredictorView
{
public:
	// Constructeur
	KWPredictorSelectiveNaiveBayesView();
	~KWPredictorSelectiveNaiveBayesView();

	// Constructeur generique
	KWPredictorView* Create() const override;

	////////////////////////////////////////////////////////
	// Methodes a reimplementer dans les sous-classes

	// Mise a jour du classifieur specifique par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par le predicteur specifique
	void EventRefresh(Object* object) override;

	// Acces au predicteur
	void SetObject(Object* object) override;
	KWPredictorSelectiveNaiveBayes* GetPredictorSelectiveNaiveBayes();
};
