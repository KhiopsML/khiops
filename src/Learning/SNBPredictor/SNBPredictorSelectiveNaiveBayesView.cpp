// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SNBPredictorSelectiveNaiveBayesView.h"

SNBPredictorSelectiveNaiveBayesView::SNBPredictorSelectiveNaiveBayesView()
{
	SNBPredictorSelectiveNaiveBayes predictor;

	// Nom de la vue (le meme que celui de l'objet edite)
	sName = predictor.GetName();

	// Parametrage pincipal de l'interface
	SetIdentifier("PredictorSelectiveNaiveBayes");
	SetLabel("Selective Naive Bayes");

	// Ajout des sous-fiches
	AddCardField("SelectionParameters", "Selection parameters", new KWSelectionParametersView);

	// On met les deux sousèfiches sans bord
	cast(UIObjectView*, GetFieldAt("TrainParameters"))->SetParameters("NoBorder");
	cast(UIObjectView*, GetFieldAt("SelectionParameters"))->SetParameters("NoBorder");
}

SNBPredictorSelectiveNaiveBayesView::~SNBPredictorSelectiveNaiveBayesView() {}

KWPredictorView* SNBPredictorSelectiveNaiveBayesView::Create() const
{
	return new SNBPredictorSelectiveNaiveBayesView;
}

void SNBPredictorSelectiveNaiveBayesView::EventUpdate(Object* object)
{
	SNBPredictorSelectiveNaiveBayes* editedObject;

	require(object != NULL);

	editedObject = cast(SNBPredictorSelectiveNaiveBayes*, object);
}

void SNBPredictorSelectiveNaiveBayesView::EventRefresh(Object* object)
{
	SNBPredictorSelectiveNaiveBayes* editedObject;

	require(object != NULL);

	editedObject = cast(SNBPredictorSelectiveNaiveBayes*, object);
}

void SNBPredictorSelectiveNaiveBayesView::SetObject(Object* object)
{
	SNBPredictorSelectiveNaiveBayes* predictor;

	require(object != NULL);

	// Acces a l'objet edite
	predictor = cast(SNBPredictorSelectiveNaiveBayes*, object);

	// Appel de la methode ancetre
	KWPredictorView::SetObject(object);

	// Parametrage des sous-fiches
	cast(KWSelectionParametersView*, GetFieldAt("SelectionParameters"))
	    ->SetObject(predictor->GetSelectionParameters());
}

SNBPredictorSelectiveNaiveBayes* SNBPredictorSelectiveNaiveBayesView::GetPredictorSelectiveNaiveBayes()
{
	return cast(SNBPredictorSelectiveNaiveBayes*, objValue);
}
