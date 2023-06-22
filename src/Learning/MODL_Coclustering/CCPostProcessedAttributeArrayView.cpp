// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-02-05 18:19:44
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "CCPostProcessedAttributeArrayView.h"

CCPostProcessedAttributeArrayView::CCPostProcessedAttributeArrayView()
{
	SetIdentifier("Array.CCPostProcessedAttribute");
	SetLabel("Coclustering variables");
	AddStringField("Type", "Type", "");
	AddStringField("Name", "Name", "");
	AddIntField("PartNumber", "Part number", 0);
	AddIntField("MaxPartNumber", "Max part number", 0);

	// Card and help prameters
	SetItemView(new CCPostProcessedAttributeView);
	CopyCardHelpTexts();

	// Parametrage des styles;
	GetFieldAt("MaxPartNumber")->SetStyle("Spinner");

	// ## Custom constructor

	cast(UIIntElement*, GetFieldAt("MaxPartNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxPartNumber"))->SetMaxValue(1000000);

	// Champ a utiliser pour les selections
	SetKeyFieldId("Name");

	// ##
}

CCPostProcessedAttributeArrayView::~CCPostProcessedAttributeArrayView()
{
	// ## Custom destructor

	// ##
}

Object* CCPostProcessedAttributeArrayView::EventNew()
{
	return new CCPostProcessedAttribute;
}

void CCPostProcessedAttributeArrayView::EventUpdate(Object* object)
{
	CCPostProcessedAttribute* editedObject;

	require(object != NULL);

	editedObject = cast(CCPostProcessedAttribute*, object);
	editedObject->SetType(GetStringValueAt("Type"));
	editedObject->SetName(GetStringValueAt("Name"));
	editedObject->SetPartNumber(GetIntValueAt("PartNumber"));
	editedObject->SetMaxPartNumber(GetIntValueAt("MaxPartNumber"));

	// ## Custom update

	// ##
}

void CCPostProcessedAttributeArrayView::EventRefresh(Object* object)
{
	CCPostProcessedAttribute* editedObject;

	require(object != NULL);

	editedObject = cast(CCPostProcessedAttribute*, object);
	SetStringValueAt("Type", editedObject->GetType());
	SetStringValueAt("Name", editedObject->GetName());
	SetIntValueAt("PartNumber", editedObject->GetPartNumber());
	SetIntValueAt("MaxPartNumber", editedObject->GetMaxPartNumber());

	// ## Custom refresh

	// ##
}

const ALString CCPostProcessedAttributeArrayView::GetClassLabel() const
{
	return "Coclustering variables";
}

// ## Method implementation

// ##
