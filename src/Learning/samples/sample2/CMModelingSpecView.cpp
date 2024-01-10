// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// Wed Jun 27 17:02:15 2007
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "CMModelingSpecView.h"

CMModelingSpecView::CMModelingSpecView()
{
	SetIdentifier("CMModelingSpec");
	SetLabel("CM Specifications");

	AddBooleanField("Majoritaire", "Majoritary classifier", true);

	// Deplacement du nouveau champ vers le haut, avant ceux de la classe ancetre
	ALString sFirstFieldId = GetFieldAtIndex(0)->GetIdentifier();
	MoveFieldBefore("Majoritaire", sFirstFieldId);

	// ## Custom constructor
}

CMModelingSpecView::~CMModelingSpecView()
{

	// ## Custom destructor

	// ##
}

void CMModelingSpecView::EventUpdate(Object* object)
{
	CMModelingSpec* editedObject;

	require(object != NULL);

	KWModelingSpecView::EventUpdate(object);
	editedObject = cast(CMModelingSpec*, object);

	editedObject->SetCMClassifier(GetBooleanValueAt("Majoritaire"));

	// ## Custom update

	// ##
}

void CMModelingSpecView::EventRefresh(Object* object)
{
	CMModelingSpec* editedObject;

	require(object != NULL);

	KWModelingSpecView::EventRefresh(object);
	editedObject = cast(CMModelingSpec*, object);

	SetBooleanValueAt("Majoritaire", editedObject->GetCMClassifier());
	// ## Custom refresh

	// ##
}

const ALString CMModelingSpecView::GetClassLabel() const
{
	return "Classifier CM";
}

// ## Method implementation

void CMModelingSpecView::SetObject(Object* object)
{
	CMModelingSpec* majorityClassifier;

	require(object != NULL);

	// Acces a l'objet edite
	majorityClassifier = cast(CMModelingSpec*, object);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

// ##
