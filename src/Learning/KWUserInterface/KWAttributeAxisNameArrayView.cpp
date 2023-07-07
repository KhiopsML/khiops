// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2019-02-25 11:58:40
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "KWAttributeAxisNameArrayView.h"

KWAttributeAxisNameArrayView::KWAttributeAxisNameArrayView()
{
	SetIdentifier("Array.KWAttributeAxisName");
	SetLabel("VariableAxes");
	AddStringField("Name", "AttributeName", "");
	AddStringField("AxisName", "AxisName", "");

	// Card and help prameters
	SetItemView(new KWAttributeAxisNameView);
	CopyCardHelpTexts();

	// ## Custom constructor

	nMaxAxisNumber = 0;

	// ##
}

KWAttributeAxisNameArrayView::~KWAttributeAxisNameArrayView()
{
	// ## Custom destructor

	// ##
}

Object* KWAttributeAxisNameArrayView::EventNew()
{
	return new KWAttributeAxisName;
}

void KWAttributeAxisNameArrayView::EventUpdate(Object* object)
{
	KWAttributeAxisName* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeAxisName*, object);
	editedObject->SetAttributeName(GetStringValueAt("Name"));
	editedObject->SetAxisName(GetStringValueAt("AxisName"));

	// ## Custom update

	// ##
}

void KWAttributeAxisNameArrayView::EventRefresh(Object* object)
{
	KWAttributeAxisName* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeAxisName*, object);
	SetStringValueAt("Name", editedObject->GetAttributeName());
	SetStringValueAt("AxisName", editedObject->GetAxisName());

	// ## Custom refresh

	// ##
}

const ALString KWAttributeAxisNameArrayView::GetClassLabel() const
{
	return "VariableAxes";
}

// ## Method implementation

void KWAttributeAxisNameArrayView::SetMaxAxisNumber(int nValue)
{
	require(nValue >= 0);
	nMaxAxisNumber = nValue;
}

int KWAttributeAxisNameArrayView::GetMaxAxisNumber() const
{
	return nMaxAxisNumber;
}

void KWAttributeAxisNameArrayView::SetAttributeLabel(const ALString& sValue)
{
	sAttributeLabel = sValue;
}

const ALString& KWAttributeAxisNameArrayView::GetAttributeLabel()
{
	return sAttributeLabel;
}

void KWAttributeAxisNameArrayView::ActionInsertItemAfter()
{
	ALString sTmp;

	// Ajout d'une variable si possible
	if (nMaxAxisNumber == 0 or GetItemNumber() < nMaxAxisNumber)
		UIObjectArrayView::ActionInsertItemAfter();
	// Message d'erreur sinon
	else
	{
		if (sAttributeLabel == "")
			AddSimpleMessage(sTmp + "Max axis number is " + IntToString(nMaxAxisNumber));
		else
			AddSimpleMessage(sTmp + "Max " + sAttributeLabel + " variable number is " +
					 IntToString(nMaxAxisNumber));
	}
}

// ##
