// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "PRWorkerView.h"

PRWorkerView::PRWorkerView()
{
	SetIdentifier("PRWorker");
	SetLabel("Employe");
	AddStringField("FirstName", "Prenom", "");
	AddStringField("FamilyName", "Nom", "");

	// ## Custom constructor

	// Ajout d'actions
	AddAction("ActionHello", "Bonjour", (ActionMethod)(&PRWorkerView::ActionHello));

	// Redefinition du libelle de l'action predefinie Exit (par defaut: OK)
	GetActionAt("Exit")->SetLabel("Quitter");

	// ##
}

PRWorkerView::~PRWorkerView()
{
	// ## Custom destructor

	// ##
}

PRWorker* PRWorkerView::GetPRWorker()
{
	require(objValue != NULL);
	return cast(PRWorker*, objValue);
}

void PRWorkerView::EventUpdate(Object* object)
{
	PRWorker* editedObject;

	require(object != NULL);

	editedObject = cast(PRWorker*, object);
	editedObject->SetFirstName(GetStringValueAt("FirstName"));
	editedObject->SetFamilyName(GetStringValueAt("FamilyName"));

	// ## Custom update

	// ##
}

void PRWorkerView::EventRefresh(Object* object)
{
	PRWorker* editedObject;

	require(object != NULL);

	editedObject = cast(PRWorker*, object);
	SetStringValueAt("FirstName", editedObject->GetFirstName());
	SetStringValueAt("FamilyName", editedObject->GetFamilyName());

	// ## Custom refresh

	// ##
}

const ALString PRWorkerView::GetClassLabel() const
{
	return "Employe";
}

// ## Method implementation

void PRWorkerView::ActionHello()
{
	PRWorker* worker;

	// Recuperation de l'objet edite
	worker = cast(PRWorker*, GetObject());
	check(worker);

	// Affichage du message
	AddSimpleMessage("Bonjour " + worker->GetFirstName());
}

// ##
