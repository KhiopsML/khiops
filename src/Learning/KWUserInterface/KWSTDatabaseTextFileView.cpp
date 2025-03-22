// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWSTDatabaseTextFileView.h"

KWSTDatabaseTextFileView::KWSTDatabaseTextFileView()
{
	SetIdentifier("KWSTDatabaseTextFile");
	SetLabel("Database");

	// Specialisation de la fiche de format, specifique au mono-table
	KWDatabaseSpecView* databaseSpecView;
	databaseSpecView = cast(KWDatabaseSpecView*, GetFieldAt("DatabaseSpec"));
	databaseSpecView->SetDataView(new KWSTDatabaseTextFileDataView);
}

KWSTDatabaseTextFileView::~KWSTDatabaseTextFileView() {}

void KWSTDatabaseTextFileView::EventUpdate(Object* object)
{
	KWDatabaseView::EventUpdate(object);
}

void KWSTDatabaseTextFileView::EventRefresh(Object* object)
{
	KWDatabaseView::EventRefresh(object);
}

const ALString KWSTDatabaseTextFileView::GetClassLabel() const
{
	return "Database";
}

KWDatabaseView* KWSTDatabaseTextFileView::Create() const
{
	return new KWSTDatabaseTextFileView;
}

ALString KWSTDatabaseTextFileView::GetTechnologyName() const
{
	return "Single table text file";
}

KWSTDatabaseTextFile* KWSTDatabaseTextFileView::GetSTDatabaseTextFile()
{
	require(objValue != NULL);
	return cast(KWSTDatabaseTextFile*, objValue);
}

void KWSTDatabaseTextFileView::AddShowFirstLinesAction()
{
	require(not IsOpened());
	require(GetActionIndex("ShowFirstLines") == -1);

	AddAction("ShowFirstLines", "Show first lines", (ActionMethod)(&KWSTDatabaseTextFileView::ShowFirstLines));
	GetActionAt("ShowFirstLines")->SetStyle("Button");

	// Info-bulles
	GetActionAt("ShowFirstLines")->SetHelpText("Show first lines of data table file in log window.");

	// Short cuts
	GetActionAt("ShowFirstLines")->SetShortCut('S');
}

void KWSTDatabaseTextFileView::ShowFirstLines()
{
	KWDatabaseFormatDetector databaseFormatDetector;

	// Utilisation du service d'affichage des premiere lignes du detecteur de format
	databaseFormatDetector.SetDatabase(GetSTDatabaseTextFile());
	databaseFormatDetector.ShowFirstLines(10);
}
