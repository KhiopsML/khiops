// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWModelingSpecView.h"

KWModelingSpecView::KWModelingSpecView()
{
	SetIdentifier("KWModelingSpec");
	SetLabel("Predictors");

	// Ajout de sous-fiches
	AddCardField("ConstructionSpec", "Feature engineering", new KWAttributeConstructionSpecView);
	AddCardField("AdvancedSpec", "Advanced predictor parameters", new KWModelingAdvancedSpecView);
	AddCardField("ExpertSpec", "Expert predictor parameters", new KWModelingExpertSpecView);

	// Fonctionnalites avancees, disponible uniquement en mode expert
	GetFieldAt("ExpertSpec")->SetVisible(GetLearningExpertMode());

	// Passage en ergonomie onglets
	SetStyle("TabbedPanes");

	// Short cuts
	GetFieldAt("ConstructionSpec")->SetShortCut('F');
	GetFieldAt("AdvancedSpec")->SetShortCut('V');
	GetFieldAt("ExpertSpec")->SetShortCut('X');
}

KWModelingSpecView::~KWModelingSpecView() {}

KWModelingSpec* KWModelingSpecView::GetKWModelingSpec()
{
	require(objValue != NULL);
	return cast(KWModelingSpec*, objValue);
}

void KWModelingSpecView::EventUpdate(Object* object)
{
	KWModelingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWModelingSpec*, object);
}

void KWModelingSpecView::EventRefresh(Object* object)
{
	KWModelingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWModelingSpec*, object);
}

const ALString KWModelingSpecView::GetClassLabel() const
{
	return "Predictors";
}

void KWModelingSpecView::SetObject(Object* object)
{
	KWModelingSpec* modelingSpec;

	require(object != NULL);

	// Acces a l'objet edite
	modelingSpec = cast(KWModelingSpec*, object);

	// Parametrages des sous-fiches par les sous-objets
	cast(KWAttributeConstructionSpecView*, GetFieldAt("ConstructionSpec"))
	    ->SetObject(modelingSpec->GetAttributeConstructionSpec());
	cast(KWModelingAdvancedSpecView*, GetFieldAt("AdvancedSpec"))->SetObject(modelingSpec);
	cast(KWModelingExpertSpecView*, GetFieldAt("ExpertSpec"))->SetObject(modelingSpec);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}
