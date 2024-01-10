// Copyright (c) 2024 Orange. All rights reserved.
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
	AddStringField("ReportFileName", "Rapport", "");

	// Parametrage des styles;
	GetFieldAt("ReportFileName")->SetStyle("FileChooser");

	// ## Custom constructor

	// Ajout de sous-fiches pour l'edition des enfants et de l'adresse personnelle
	AddListField("Children", "Enfants", new PRChildArrayView);
	AddCardField("PersonalAddress", "Adresse personnelle", new PRAddressView);

	// Passage en ergonomie onglets (par defaut: sous-fiches directement dans la vue)
	SetStyle("TabbedPanes");

	// Ajout d'actions
	AddAction("ActionWriteReport", "Ecriture d'un rapport", (ActionMethod)(&PRWorkerView::ActionWriteReport));
	AddAction("ActionInspectProfessionnalAddress", "Adresse professionnelle...",
		  (ActionMethod)(&PRWorkerView::ActionInspectProfessionnalAddress));

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
	editedObject->SetReportFileName(GetStringValueAt("ReportFileName"));

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
	SetStringValueAt("ReportFileName", editedObject->GetReportFileName());

	// ## Custom refresh

	// ##
}

const ALString PRWorkerView::GetClassLabel() const
{
	return "Employe";
}

// ## Method implementation

void PRWorkerView::ActionWriteReport()
{
	PRWorker* worker;

	// Recuperation de l'objet edite
	worker = GetWorker();

	// Message d'information utilisateur
	AddSimpleMessage("Ecriture du rapport " + GetStringValueAt("ReportFileName"));

	// Ecriture du rapport
	worker->WriteReport();
}

void PRWorkerView::ActionInspectProfessionnalAddress()
{
	PRWorker* worker;
	PRAddressView addressView;

	// Recuperation de l'objet edite
	worker = GetWorker();

	// Affichage de l'adresse professionnelle dans une sous fenetre
	addressView.SetLabel("Adresse professionnelle");
	addressView.SetObject(worker->GetProfessionalAddress());
	addressView.Open();
}

void PRWorkerView::SetObject(Object* object)
{
	PRWorker* worker;

	require(object != NULL);

	// Acces a l'objet edite
	worker = cast(PRWorker*, object);

	// Parametrage des sous-fiches
	cast(PRChildArrayView*, GetFieldAt("Children"))->SetObjectArray(worker->GetChildren());
	cast(PRAddressView*, GetFieldAt("PersonalAddress"))->SetObject(worker->GetPersonalAddress());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

PRWorker* PRWorkerView::GetWorker()
{
	return GetPRWorker();
}

// ##
