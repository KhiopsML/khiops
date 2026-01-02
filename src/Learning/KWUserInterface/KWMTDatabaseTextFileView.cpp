// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWMTDatabaseTextFileView.h"

KWMTDatabaseTextFileView::KWMTDatabaseTextFileView()
{
	SetIdentifier("KWMTDatabaseTextFile");
	SetLabel("Database");

	// Specialisation de la fiche de format, specifique au mono-table
	KWDatabaseSpecView* databaseSpecView;
	databaseSpecView = cast(KWDatabaseSpecView*, GetFieldAt("DatabaseSpec"));
	databaseSpecView->SetDataView(new KWMTDatabaseTextFileDataView);
}

KWMTDatabaseTextFileView::~KWMTDatabaseTextFileView() {}

void KWMTDatabaseTextFileView::EventUpdate(Object* object)
{
	KWDatabaseView::EventUpdate(object);
}

void KWMTDatabaseTextFileView::EventRefresh(Object* object)
{
	KWDatabaseView::EventRefresh(object);
}

const ALString KWMTDatabaseTextFileView::GetClassLabel() const
{
	return "Database";
}

boolean KWMTDatabaseTextFileView::IsMultiTableTechnology() const
{
	return true;
}

void KWMTDatabaseTextFileView::SetEditableTableNumber(int nValue)
{
	KWMTDatabaseTextFileDataView* dataView;

	require(nValue >= 1);

	// Parametrage du nombre tables editables dans la fiche de format multi-table de la base
	dataView = cast(KWMTDatabaseTextFileDataView*, GetDataView());
	cast(UIObjectArrayView*, dataView->GetFieldAt("DatabaseFiles"))->SetLineNumber(nValue);
}

int KWMTDatabaseTextFileView::GetEditableTableNumber() const
{
	KWMTDatabaseTextFileView* databaseTextFileView;
	KWMTDatabaseTextFileDataView* dataView;

	// Acces au du nombre tables editables dans la fiche de format multi-table de la base
	// On doit passer par une variable intermediaire non const pour acceder a la vue de format
	databaseTextFileView = cast(KWMTDatabaseTextFileView*, this);
	dataView = cast(KWMTDatabaseTextFileDataView*, databaseTextFileView->GetDataView());
	return cast(UIObjectArrayView*, dataView->GetFieldAt("DatabaseFiles"))->GetLineNumber();
}

void KWMTDatabaseTextFileView::ToWriteOnlyMode()
{
	KWDatabaseSpecView* databaseSpecView;
	KWMTDatabaseTextFileDataView* dataView;

	// Appel de la methode ancetre
	KWDatabaseView::ToWriteOnlyMode();

	// Parametrage de la fiche de format multi-table de la base
	databaseSpecView = cast(KWDatabaseSpecView*, GetFieldAt("DatabaseSpec"));
	dataView = cast(KWMTDatabaseTextFileDataView*, databaseSpecView->GetFieldAt("Data"));
	dataView->ToWriteOnlyMode();
}

KWDatabaseView* KWMTDatabaseTextFileView::Create() const
{
	return new KWMTDatabaseTextFileView;
}

ALString KWMTDatabaseTextFileView::GetTechnologyName() const
{
	return "Multiple table text file";
}

KWMTDatabaseTextFile* KWMTDatabaseTextFileView::GetMTDatabaseTextFile()
{
	require(objValue != NULL);
	return cast(KWMTDatabaseTextFile*, objValue);
}
