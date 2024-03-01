// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseSpecView.h"

KWDatabaseSpecView::KWDatabaseSpecView()
{
	// Identifiant
	SetIdentifier("KWDatabase");
	SetLabel("Database");

	// Creation des sous-fenetres
	AddCardField("Sampling", "Sampling", new KWDatabaseSamplingView);
	AddCardField("Selection", "Selection", new KWDatabaseSelectionView);

	// Passage en ergonomie onglet, et sans bord
	SetParameters("NoBorder");
	SetStyle("TabbedPanes");
}

KWDatabaseSpecView::~KWDatabaseSpecView() {}

void KWDatabaseSpecView::SetDataView(UIObjectView* formatView)
{
	require(GetFieldIndex("Data") == -1);
	require(formatView != NULL);

	// Ajout de la sous-fenetre, en lui donnant un libelle plus generique
	AddCardField("Data", "Database", formatView);

	// Deplacement en tete
	MoveFieldBefore("Data", "Sampling");
}

UIObjectView* KWDatabaseSpecView::GetDataView()
{
	return cast(UIObjectView*, GetFieldAt("Data"));
}

void KWDatabaseSpecView::ToBasicReadMode()
{
	UIObjectView* dataView;

	require(Check());

	// Parametrage de la visibilite du detecteur de format
	dataView = cast(UIObjectView*, GetFieldAt("Data"));
	if (dataView->GetFieldIndex("DatabaseFormatDetector") >= 0)
		dataView->GetFieldAt("DatabaseFormatDetector")->SetVisible(true);

	// Parametrage de la visibilite des fiches de Sampling et Selection
	GetFieldAt("Sampling")->SetVisible(false);
	GetFieldAt("Selection")->SetVisible(false);

	// Supression des onglets
	SetStyle("");
	dataView->SetParameters("NoBorder");
}

void KWDatabaseSpecView::ToWriteOnlyMode()
{
	UIObjectView* dataView;

	require(Check());

	// Comme en mode basique
	ToBasicReadMode();

	// Mais sans le detecteur de format
	dataView = cast(UIObjectView*, GetFieldAt("Data"));
	if (dataView->GetFieldIndex("DatabaseFormatDetector") >= 0)
		dataView->GetFieldAt("DatabaseFormatDetector")->SetVisible(false);
}

KWDatabase* KWDatabaseSpecView::GetKWDatabase()
{
	require(objValue != NULL);
	return cast(KWDatabase*, objValue);
}

void KWDatabaseSpecView::EventUpdate(Object* object)
{
	require(Check());
}

void KWDatabaseSpecView::EventRefresh(Object* object)
{
	require(Check());
}

const ALString KWDatabaseSpecView::GetClassLabel() const
{
	return "Database";
}

boolean KWDatabaseSpecView::Check() const
{
	return GetFieldIndex("Data") == 0;
}

void KWDatabaseSpecView::SetObject(Object* object)
{
	KWDatabase* database;

	require(Check());
	require(object != NULL);

	// Acces a l'objet edite
	database = cast(KWDatabase*, object);

	// Parametrage des fiches
	cast(UIObjectView*, GetFieldAt("Data"))->SetObject(database);
	cast(UIObjectView*, GetFieldAt("Sampling"))->SetObject(database);
	cast(UIObjectView*, GetFieldAt("Selection"))->SetObject(database);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}
