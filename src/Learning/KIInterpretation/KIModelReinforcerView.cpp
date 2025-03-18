// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIModelReinforcerView.h"

KIModelReinforcerView::KIModelReinforcerView()
{
	SetIdentifier("KIModelReinforcer");
	SetLabel("Reinforce model");
	AddStringField("ReinforcedTargetValue", "Target value to reinforce", "");

	// ## Custom constructor

	// ##
}

KIModelReinforcerView::~KIModelReinforcerView()
{
	// ## Custom destructor

	// ##
}

KIModelReinforcer* KIModelReinforcerView::GetKIModelReinforcer()
{
	require(objValue != NULL);
	return cast(KIModelReinforcer*, objValue);
}

void KIModelReinforcerView::EventUpdate(Object* object)
{
	KIModelReinforcer* editedObject;

	require(object != NULL);

	KIModelServiceView::EventUpdate(object);
	editedObject = cast(KIModelReinforcer*, object);
	editedObject->SetReinforcedTargetValue(GetStringValueAt("ReinforcedTargetValue"));

	// ## Custom update

	// ##
}

void KIModelReinforcerView::EventRefresh(Object* object)
{
	KIModelReinforcer* editedObject;

	require(object != NULL);

	KIModelServiceView::EventRefresh(object);
	editedObject = cast(KIModelReinforcer*, object);
	SetStringValueAt("ReinforcedTargetValue", editedObject->GetReinforcedTargetValue());

	// ## Custom refresh

	// ##
}

const ALString KIModelReinforcerView::GetClassLabel() const
{
	return "Reinforce model";
}

// ## Method implementation

// ##
