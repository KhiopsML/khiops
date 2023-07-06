// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CMModelingSpecView.h"

CMModelingSpecView::CMModelingSpecView()
{
	SetIdentifier("CMModelingSpec");
	SetLabel("Classifier specifications");

	AddBooleanField("MajorityClassifier", "Majority classifier", true);

	// Deplacement du nouveau champ vers le haut, avant ceux de la classe ancetre
	ALString sFirstFieldId = GetFieldAtIndex(0)->GetIdentifier();
	MoveFieldBefore("MajorityClassifier", sFirstFieldId);
}

CMModelingSpecView::~CMModelingSpecView() {}

void CMModelingSpecView::EventUpdate(Object* object)
{
	CMModelingSpec* editedObject;

	require(object != NULL);

	KWModelingSpecView::EventUpdate(object);
	editedObject = cast(CMModelingSpec*, object);

	editedObject->SetTrainMajorityClassifier(GetBooleanValueAt("MajorityClassifier"));
}

void CMModelingSpecView::EventRefresh(Object* object)
{
	CMModelingSpec* editedObject;

	require(object != NULL);

	KWModelingSpecView::EventRefresh(object);
	editedObject = cast(CMModelingSpec*, object);

	SetBooleanValueAt("MajorityClassifier", editedObject->GetTrainMajorityClassifier());
}

const ALString CMModelingSpecView::GetClassLabel() const
{
	return "Majority classifier";
}

void CMModelingSpecView::SetObject(Object* object)
{
	CMModelingSpec* majorityClassifier;

	require(object != NULL);

	// Acces a l'objet edite
	majorityClassifier = cast(CMModelingSpec*, object);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}
