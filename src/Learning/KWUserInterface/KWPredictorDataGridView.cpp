// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorDataGridView.h"

KWPredictorDataGridView::KWPredictorDataGridView()
{
	KWPredictorDataGrid predictor;

	// Nom de la vue (le meme que celui de l'objet edite)
	sName = predictor.GetName();

	// Parametrage principal de l'interface
	SetIdentifier("PredictorDataGrid");
	SetLabel("Data grid");

	// Ajout des sous-fiches
	AddCardField("DataGridOptimizerParameters", "Data grid parameters", new KWDataGridOptimizerParametersView);
}

KWPredictorDataGridView::~KWPredictorDataGridView() {}

KWPredictorView* KWPredictorDataGridView::Create() const
{
	return new KWPredictorDataGridView;
}

void KWPredictorDataGridView::EventUpdate(Object* object)
{
	require(object != NULL);
}

void KWPredictorDataGridView::EventRefresh(Object* object)
{
	require(object != NULL);
}

void KWPredictorDataGridView::SetObject(Object* object)
{
	KWPredictorDataGrid* predictor;

	require(object != NULL);

	// Acces a l'objet edite
	predictor = cast(KWPredictorDataGrid*, object);

	// Appel de la methode ancetre
	KWPredictorView::SetObject(object);

	// Parametrage des sous-fiches
	cast(KWDataGridOptimizerParametersView*, GetFieldAt("DataGridOptimizerParameters"))
	    ->SetObject(predictor->GetDataGridOptimizerParameters());
}

KWPredictorDataGrid* KWPredictorDataGridView::GetPredictorDataGrid()
{
	return cast(KWPredictorDataGrid*, objValue);
}
