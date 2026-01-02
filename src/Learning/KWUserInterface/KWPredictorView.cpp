// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorView.h"

KWPredictorView::KWPredictorView()
{
	sName = "Predictor";

	// Ajout des sous-fiches
	AddCardField("TrainParameters", "Train parameters", new KWTrainParametersView);
}

KWPredictorView::~KWPredictorView() {}

KWPredictorView* KWPredictorView::Create() const
{
	return new KWPredictorView;
}

const ALString& KWPredictorView::GetName() const
{
	return sName;
}

const ALString KWPredictorView::GetClassLabel() const
{
	return GetName();
}

void KWPredictorView::EventUpdate(Object* object) {}

void KWPredictorView::EventRefresh(Object* object) {}

void KWPredictorView::SetObject(Object* object)
{
	KWPredictor* predictor;

	require(object != NULL);

	// Acces a l'objet edite
	predictor = cast(KWPredictor*, object);

	// Parametrage des sous-fiches
	cast(KWTrainParametersView*, GetFieldAt("TrainParameters"))->SetObject(predictor->GetTrainParameters());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

KWPredictor* KWPredictorView::GetPredictor()
{
	return cast(KWPredictor*, objValue);
}

///////////////////////////////////////////////////////////////////////////

void KWPredictorView::RegisterPredictorView(KWPredictorView* predictorView)
{
	require(predictorView != NULL);
	require(predictorView->GetName() != "");
	require(KWClass::CheckName(predictorView->GetName(), KWClass::None, predictorView));
	require(odPredictorViews == NULL or odPredictorViews->Lookup(predictorView->GetName()) == NULL);

	// Creation si necessaire du dictionnaire de predictorViews
	if (odPredictorViews == NULL)
		odPredictorViews = new ObjectDictionary;

	// Remplacement par le nouveau predictorView
	odPredictorViews->SetAt(predictorView->GetName(), predictorView);
}

KWPredictorView* KWPredictorView::LookupPredictorView(const ALString& sName)
{
	// Creation si necessaire du dictionnaire de predictorViews
	if (odPredictorViews == NULL)
		odPredictorViews = new ObjectDictionary;

	return cast(KWPredictorView*, odPredictorViews->Lookup(sName));
}

KWPredictorView* KWPredictorView::ClonePredictorView(const ALString& sName)
{
	KWPredictorView* referencePredictorView;

	// Creation si necessaire du dictionnaire de predictorViews
	if (odPredictorViews == NULL)
		odPredictorViews = new ObjectDictionary;

	// Recherche d'un predictorView de meme nom
	referencePredictorView = cast(KWPredictorView*, odPredictorViews->Lookup(sName));

	// Retour de son Clone si possible
	if (referencePredictorView == NULL)
		return NULL;
	else
		return referencePredictorView->Create();
}

void KWPredictorView::ExportAllPredictorViews(ObjectArray* oaPredictorViews)
{
	require(oaPredictorViews != NULL);

	// Creation si necessaire du dictionnaire de predictorViews
	if (odPredictorViews == NULL)
		odPredictorViews = new ObjectDictionary;

	// Tri des vues avant de retourner le tableau
	odPredictorViews->ExportObjectArray(oaPredictorViews);
	oaPredictorViews->SetCompareFunction(KWPredictorViewCompareName);
	oaPredictorViews->Sort();
}

void KWPredictorView::DeleteAllPredictorViews()
{
	if (odPredictorViews != NULL)
	{
		odPredictorViews->DeleteAll();
		delete odPredictorViews;
		odPredictorViews = NULL;
	}
	ensure(odPredictorViews == NULL);
}

ObjectDictionary* KWPredictorView::odPredictorViews = NULL;

int KWPredictorViewCompareName(const void* first, const void* second)
{
	KWPredictorView* aFirst;
	KWPredictorView* aSecond;
	int nResult;

	aFirst = cast(KWPredictorView*, *(Object**)first);
	aSecond = cast(KWPredictorView*, *(Object**)second);
	nResult = aFirst->GetName().Compare(aSecond->GetName());
	return nResult;
}
