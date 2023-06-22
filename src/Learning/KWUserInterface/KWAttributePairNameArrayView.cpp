// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-04-25 11:10:57
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "KWAttributePairNameArrayView.h"

KWAttributePairNameArrayView::KWAttributePairNameArrayView()
{
	SetIdentifier("Array.KWAttributePairName");
	SetLabel("Variable pairs");
	AddStringField("FirstName", "First name", "");
	AddStringField("SecondName", "Second name", "");

	// Card and help prameters
	SetItemView(new KWAttributePairNameView);
	CopyCardHelpTexts();

	// ## Custom constructor

	// Nombre max de paires de variable
	nMaxAttributePairNumber = 0;

	// Ajout des action
	AddAction("RemoveAllPairs", "Clear pair list", (ActionMethod)(&KWAttributePairNameArrayView::RemoveAllPairs));
	GetActionAt("RemoveAllPairs")->SetStyle("Button");

	// Info-bulles
	GetActionAt("RemoveAllPairs")->SetHelpText("Remove all specific variable pairs.");

	// Short cuts
	GetActionAt("RemoveAllPairs")->SetShortCut('C');

	// ##
}

KWAttributePairNameArrayView::~KWAttributePairNameArrayView()
{
	// ## Custom destructor

	// ##
}

Object* KWAttributePairNameArrayView::EventNew()
{
	return new KWAttributePairName;
}

void KWAttributePairNameArrayView::EventUpdate(Object* object)
{
	KWAttributePairName* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributePairName*, object);
	editedObject->SetFirstName(GetStringValueAt("FirstName"));
	editedObject->SetSecondName(GetStringValueAt("SecondName"));

	// ## Custom update

	// ##
}

void KWAttributePairNameArrayView::EventRefresh(Object* object)
{
	KWAttributePairName* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributePairName*, object);
	SetStringValueAt("FirstName", editedObject->GetFirstName());
	SetStringValueAt("SecondName", editedObject->GetSecondName());

	// ## Custom refresh

	// ##
}

const ALString KWAttributePairNameArrayView::GetClassLabel() const
{
	return "Variable pairs";
}

// ## Method implementation

void KWAttributePairNameArrayView::RemoveAllPairs()
{
	// Supression des paires
	GetObjectArray()->DeleteAll();
}

void KWAttributePairNameArrayView::SetMaxAttributePairNumber(int nValue)
{
	require(nValue >= 0);
	nMaxAttributePairNumber = nValue;
}

int KWAttributePairNameArrayView::GetMaxAttributePairNumber() const
{
	return nMaxAttributePairNumber;
}

void KWAttributePairNameArrayView::ActionInsertItemAfter()
{
	ALString sTmp;

	// Ajout d'une variable si possible
	if (nMaxAttributePairNumber == 0 or GetItemNumber() < nMaxAttributePairNumber)
	{
		UIObjectArrayView::ActionInsertItemAfter();
	}
	// Message d'erreur sinon
	else
	{
		AddSimpleMessage(sTmp + "Limit of maximum number of variable pairs is " +
				 IntToString(nMaxAttributePairNumber));
	}
}

// ##
