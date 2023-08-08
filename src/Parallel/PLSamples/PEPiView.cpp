// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "PEPiView.h"

PEPiView::PEPiView()
{
	SetIdentifier("PEPi");
	SetLabel("Pi parallel computation");
	AddIntField("IterationNumber", "Number of iterations", 0);
	AddStringField("Pi", "Pi approximation", "");
	AddDoubleField("ElapsedTime", "Computation time", 0);

	// ## Custom constructor
	int nMaxCores;

	// Champ specifique sur la limite du nombre de processeur
	// synchronise avec la classe RMResourceConstraints
	AddIntField("ProcessusNumber", "Number of process", 0);
	GetFieldAt("ProcessusNumber")->SetStyle("Spinner");

	// Limites du nombre de processor
	nMaxCores = max(RMResourceManager::GetLogicalProcessNumber() - 1, 1);
	RMResourceConstraints::SetMaxCoreNumberOnCluster(nMaxCores);
	cast(UIIntElement*, GetFieldAt("ProcessusNumber"))->SetMinValue(1);
	cast(UIIntElement*, GetFieldAt("ProcessusNumber"))->SetMaxValue(nMaxCores);
	cast(UIIntElement*, GetFieldAt("ProcessusNumber"))->SetDefaultValue(1);

	// Resultats en read-only
	GetFieldAt("Pi")->SetEditable(false);
	GetFieldAt("ElapsedTime")->SetEditable(false);

	// Declaration des actions
	AddAction("ComputePi", "Compute pi in parallel", (ActionMethod)(&PEPiView::ComputePi));
	GetActionAt("ComputePi")->SetStyle("Button");

	// ##
}

PEPiView::~PEPiView()
{
	// ## Custom destructor

	// ##
}

PEPi* PEPiView::GetPEPi()
{
	require(objValue != NULL);
	return cast(PEPi*, objValue);
}

void PEPiView::EventUpdate(Object* object)
{
	PEPi* editedObject;

	require(object != NULL);

	editedObject = cast(PEPi*, object);
	editedObject->SetIterationNumber(GetIntValueAt("IterationNumber"));
	editedObject->SetPi(GetStringValueAt("Pi"));
	editedObject->SetElapsedTime(GetDoubleValueAt("ElapsedTime"));

	// ## Custom update

	// Synchronisation avec la gestion des contraintes
	RMResourceConstraints::SetMaxCoreNumberOnCluster(GetIntValueAt("ProcessusNumber"));

	// ##
}

void PEPiView::EventRefresh(Object* object)
{
	PEPi* editedObject;

	require(object != NULL);

	editedObject = cast(PEPi*, object);
	SetIntValueAt("IterationNumber", editedObject->GetIterationNumber());
	SetStringValueAt("Pi", editedObject->GetPi());
	SetDoubleValueAt("ElapsedTime", editedObject->GetElapsedTime());

	// ## Custom refresh

	// Synchronisation avec la gestion des contraintes
	SetIntValueAt("ProcessusNumber", RMResourceConstraints::GetMaxCoreNumberOnCluster());

	// ##
}

const ALString PEPiView::GetClassLabel() const
{
	return "Pi parallel computation";
}

// ## Method implementation

void PEPiView::ComputePi()
{
	PEPi* pi;

	// Acces a l'objet edite
	pi = cast(PEPi*, GetObject());

	// Calcul de Pi
	pi->ComputePi();
	AddMessage(IntToString(RMResourceConstraints::GetMaxCoreNumberOnCluster()));
	AddMessage("Pi = " + pi->GetPi());
	AddMessage(SecondsToString(pi->GetElapsedTime()));
}

void PEPiView::Test(int argv, char** argc)
{
	PEPiView view;
	PEPi pi;

	// MemSetAllocIndexExit(2446);

	// Analyse de la ligne de commande
	UIObject::SetUIMode(UIObject::Graphic);

	// Ouverture de la vue
	view.SetObject(&pi);
	view.Open();
}

// ##
