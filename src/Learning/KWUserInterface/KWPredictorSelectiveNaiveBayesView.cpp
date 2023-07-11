// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorSelectiveNaiveBayesView.h"

KWPredictorSelectiveNaiveBayesView::KWPredictorSelectiveNaiveBayesView()
{
	KWPredictorSelectiveNaiveBayes predictor;

	// Nom de la vue (le meme que celui de l'objet edite)
	sName = predictor.GetName();

	// Parametrage principal de l'interface
	SetIdentifier("PredictorSelectiveNaiveBayes");
	SetLabel("Selective Naive Bayes");

	// Ajout des sous-fiches
	AddCardField("SelectionParameters", "Selection parameters", new KWSelectionParametersView);
}

KWPredictorSelectiveNaiveBayesView::~KWPredictorSelectiveNaiveBayesView() {}

KWPredictorView* KWPredictorSelectiveNaiveBayesView::Create() const
{
	return new KWPredictorSelectiveNaiveBayesView;
}

void KWPredictorSelectiveNaiveBayesView::EventUpdate(Object* object)
{
	KWPredictorSelectiveNaiveBayes* editedObject;

	require(object != NULL);

	editedObject = cast(KWPredictorSelectiveNaiveBayes*, object);
}

void KWPredictorSelectiveNaiveBayesView::EventRefresh(Object* object)
{
	KWPredictorSelectiveNaiveBayes* editedObject;

	require(object != NULL);

	editedObject = cast(KWPredictorSelectiveNaiveBayes*, object);
}

void KWPredictorSelectiveNaiveBayesView::SetObject(Object* object)
{
	KWPredictorSelectiveNaiveBayes* predictor;

	require(object != NULL);

	// Acces a l'objet edite
	predictor = cast(KWPredictorSelectiveNaiveBayes*, object);

	// Appel de la methode ancetre
	KWPredictorView::SetObject(object);

	// Parametrage des sous-fiches
	cast(KWSelectionParametersView*, GetFieldAt("SelectionParameters"))
	    ->SetObject(predictor->GetSelectionParameters());
}

KWPredictorSelectiveNaiveBayes* KWPredictorSelectiveNaiveBayesView::GetPredictorSelectiveNaiveBayes()
{
	return cast(KWPredictorSelectiveNaiveBayes*, objValue);
}
