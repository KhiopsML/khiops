// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWBenchmarkSpecArrayView.h"

KWBenchmarkSpecArrayView::KWBenchmarkSpecArrayView()
{
	SetIdentifier("Array.KWBenchmarkSpec");
	SetLabel("Benchmarks");
	AddStringField("ClassName", "Dictionary", "");
	AddStringField("TargetAttributeName", "Target variable", "");
	AddStringField("DatabaseName", "Database file", "");

	// Card and help prameters
	SetItemView(new KWBenchmarkSpecView);
	CopyCardHelpTexts();

	// ## Custom constructor

	// Champ a utiliser pour les selections
	SetKeyFieldId("ClassName");

	// ##
}

KWBenchmarkSpecArrayView::~KWBenchmarkSpecArrayView()
{
	// ## Custom destructor

	// ##
}

Object* KWBenchmarkSpecArrayView::EventNew()
{
	return new KWBenchmarkSpec;
}

void KWBenchmarkSpecArrayView::EventUpdate(Object* object)
{
	KWBenchmarkSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWBenchmarkSpec*, object);

	// ## Custom update

	// ##
}

void KWBenchmarkSpecArrayView::EventRefresh(Object* object)
{
	KWBenchmarkSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWBenchmarkSpec*, object);
	SetStringValueAt("ClassName", editedObject->GetClassName());
	SetStringValueAt("TargetAttributeName", editedObject->GetTargetAttributeName());
	SetStringValueAt("DatabaseName", editedObject->GetDatabaseName());

	// ## Custom refresh

	// ##
}

const ALString KWBenchmarkSpecArrayView::GetClassLabel() const
{
	return "Benchmarks";
}

// ## Method implementation

// ##
