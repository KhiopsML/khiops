// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAttributeNameArrayView.h"

KWAttributeNameArrayView::KWAttributeNameArrayView()
{
	SetIdentifier("Array.KWAttributeName");
	SetLabel("Variables");
	AddStringField("Name", "Name", "");

	// Card and help prameters
	SetItemView(new KWAttributeNameView);
	CopyCardHelpTexts();

	// ## Custom constructor

	nMaxAttributeNumber = 0;

	// ##
}

KWAttributeNameArrayView::~KWAttributeNameArrayView()
{
	// ## Custom destructor

	// ##
}

Object* KWAttributeNameArrayView::EventNew()
{
	return new KWAttributeName;
}

void KWAttributeNameArrayView::EventUpdate(Object* object)
{
	KWAttributeName* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeName*, object);
	editedObject->SetName(GetStringValueAt("Name"));

	// ## Custom update

	// ##
}

void KWAttributeNameArrayView::EventRefresh(Object* object)
{
	KWAttributeName* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeName*, object);
	SetStringValueAt("Name", editedObject->GetName());

	// ## Custom refresh

	// ##
}

const ALString KWAttributeNameArrayView::GetClassLabel() const
{
	return "Variables";
}

// ## Method implementation

void KWAttributeNameArrayView::SetMaxAttributeNumber(int nValue)
{
	require(nValue >= 0);
	nMaxAttributeNumber = nValue;
}

int KWAttributeNameArrayView::GetMaxAttributeNumber() const
{
	return nMaxAttributeNumber;
}

void KWAttributeNameArrayView::SetAttributeLabel(const ALString& sValue)
{
	sAttributeLabel = sValue;
}

const ALString& KWAttributeNameArrayView::GetAttributeLabel()
{
	return sAttributeLabel;
}

void KWAttributeNameArrayView::ActionInsertItemAfter()
{
	ALString sTmp;

	// Ajout d'une variable si possible
	if (nMaxAttributeNumber == 0 or GetItemNumber() < nMaxAttributeNumber)
		UIObjectArrayView::ActionInsertItemAfter();
	// Message d'erreur sinon
	else
	{
		if (sAttributeLabel == "")
			AddSimpleMessage(sTmp + "Max variable number is " + IntToString(nMaxAttributeNumber));
		else
			AddSimpleMessage(sTmp + "Max " + sAttributeLabel + " variable number is " +
					 IntToString(nMaxAttributeNumber));
	}
}

// ##