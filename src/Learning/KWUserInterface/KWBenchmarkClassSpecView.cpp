// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWBenchmarkClassSpecView.h"

KWBenchmarkClassSpecView::KWBenchmarkClassSpecView()
{
	SetIdentifier("KWBenchmarkClassSpec");
	SetLabel("Benchmark dictionary");
	AddStringField("ClassFileName", "Dictionary file", "");
	AddStringField("ClassName", "Dictionary", "");
	AddStringField("TargetAttributeName", "Target variable", "");

	// Parametrage des styles;
	GetFieldAt("ClassFileName")->SetStyle("FileChooser");

	// ## Custom constructor

	// Specialisation des extensions
	GetFieldAt("ClassFileName")->SetParameters("Dictionary\nkdic");

	// On indique que le champ de parametrage du dictionnaire declenche une action de rafraichissement
	// de l'interface immediatement apres une mise a jour, pour pouvouir rafraichir les mapping des databases
	cast(UIElement*, GetFieldAt("ClassName"))->SetTriggerRefresh(true);

	// ##
}

KWBenchmarkClassSpecView::~KWBenchmarkClassSpecView()
{
	// ## Custom destructor

	// ##
}

KWBenchmarkClassSpec* KWBenchmarkClassSpecView::GetKWBenchmarkClassSpec()
{
	require(objValue != NULL);
	return cast(KWBenchmarkClassSpec*, objValue);
}

void KWBenchmarkClassSpecView::EventUpdate(Object* object)
{
	KWBenchmarkClassSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWBenchmarkClassSpec*, object);
	editedObject->SetClassFileName(GetStringValueAt("ClassFileName"));
	editedObject->SetClassName(GetStringValueAt("ClassName"));
	editedObject->SetTargetAttributeName(GetStringValueAt("TargetAttributeName"));

	// ## Custom update

	// ##
}

void KWBenchmarkClassSpecView::EventRefresh(Object* object)
{
	KWBenchmarkClassSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWBenchmarkClassSpec*, object);
	SetStringValueAt("ClassFileName", editedObject->GetClassFileName());
	SetStringValueAt("ClassName", editedObject->GetClassName());
	SetStringValueAt("TargetAttributeName", editedObject->GetTargetAttributeName());

	// ## Custom refresh

	// ##
}

const ALString KWBenchmarkClassSpecView::GetClassLabel() const
{
	return "Benchmark dictionary";
}

// ## Method implementation

// ##
