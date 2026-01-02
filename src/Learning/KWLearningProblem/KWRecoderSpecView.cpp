// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWRecoderSpecView.h"

KWRecoderSpecView::KWRecoderSpecView()
{
	SetIdentifier("KWRecoderSpec");
	SetLabel("Recoders");
	AddBooleanField("Recoder", "Build recoder", false);

	// Parametrage des styles;
	GetFieldAt("Recoder")->SetStyle("CheckBox");

	// ## Custom constructor

	// Ajout de sous-fiches
	AddCardField("RecodingSpec", "Recoding parameters", new KWRecodingSpecView);

	// Info-bulles
	GetFieldAt("Recoder")->SetHelpText("Build a recoding dictionary that recodes the input database"
					   "\n with a subset of initial or preprocessed variables.");

	// ##
}

KWRecoderSpecView::~KWRecoderSpecView()
{
	// ## Custom destructor

	// ##
}

KWRecoderSpec* KWRecoderSpecView::GetKWRecoderSpec()
{
	require(objValue != NULL);
	return cast(KWRecoderSpec*, objValue);
}

void KWRecoderSpecView::EventUpdate(Object* object)
{
	KWRecoderSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWRecoderSpec*, object);
	editedObject->SetRecoder(GetBooleanValueAt("Recoder"));

	// ## Custom update

	// ##
}

void KWRecoderSpecView::EventRefresh(Object* object)
{
	KWRecoderSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWRecoderSpec*, object);
	SetBooleanValueAt("Recoder", editedObject->GetRecoder());

	// ## Custom refresh

	// ##
}

const ALString KWRecoderSpecView::GetClassLabel() const
{
	return "Recoders";
}

// ## Method implementation

void KWRecoderSpecView::SetObject(Object* object)
{
	KWRecoderSpec* recoderSpec;

	require(object != NULL);

	// Acces a l'objet edite
	recoderSpec = cast(KWRecoderSpec*, object);

	// Parametrages des sous-fiches par les sous-objets
	cast(KWRecodingSpecView*, GetFieldAt("RecodingSpec"))->SetObject(recoderSpec->GetRecodingSpec());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

// ##
