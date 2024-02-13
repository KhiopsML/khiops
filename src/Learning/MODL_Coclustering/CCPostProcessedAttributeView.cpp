// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCPostProcessedAttributeView.h"

CCPostProcessedAttributeView::CCPostProcessedAttributeView()
{
	SetIdentifier("CCPostProcessedAttribute");
	SetLabel("Coclustering variable");
	AddStringField("Type", "Type", "");
	AddStringField("Name", "Name", "");
	AddIntField("PartNumber", "Part number", 0);
	AddIntField("MaxPartNumber", "Max part number", 0);

	// Parametrage des styles;
	GetFieldAt("MaxPartNumber")->SetStyle("Spinner");

	// ## Custom constructor

	cast(UIIntElement*, GetFieldAt("MaxPartNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxPartNumber"))->SetMaxValue(1000000);

	// Info-bulles
	GetFieldAt("Type")->SetHelpText("Type of the input coclustering variable.");
	GetFieldAt("Name")->SetHelpText("Name of the input coclustering variable.");
	GetFieldAt("PartNumber")->SetHelpText("Part number of the input coclustering variable.");
	GetFieldAt("MaxPartNumber")
	    ->SetHelpText("Max number of parts to keep for this variable"
			  "\n in the simplified coclustering (0: no constraint).");

	// ##
}

CCPostProcessedAttributeView::~CCPostProcessedAttributeView()
{
	// ## Custom destructor

	// ##
}

CCPostProcessedAttribute* CCPostProcessedAttributeView::GetCCPostProcessedAttribute()
{
	require(objValue != NULL);
	return cast(CCPostProcessedAttribute*, objValue);
}

void CCPostProcessedAttributeView::EventUpdate(Object* object)
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

void CCPostProcessedAttributeView::EventRefresh(Object* object)
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

const ALString CCPostProcessedAttributeView::GetClassLabel() const
{
	return "Coclustering variable";
}

// ## Method implementation

// ##
