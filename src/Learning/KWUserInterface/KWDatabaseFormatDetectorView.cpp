// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseFormatDetectorView.h"

KWDatabaseFormatDetectorView::KWDatabaseFormatDetectorView()
{
	// Parametrage general
	SetIdentifier("KWDatabaseFormatDetectorView");
	SetLabel("Database format detector");

	// Ajout de l'action de detetection de format
	AddAction("DetectFileFormat", "Detect file format",
		  (ActionMethod)(&KWDatabaseFormatDetectorView::DetectFileFormat));

	// Maquettage en petit bouton dans une fiche sans bordure
	SetParameters("NoBorder");
	GetActionAt("DetectFileFormat")->SetStyle("SmallButton");

	// Short cuts
	GetActionAt("DetectFileFormat")->SetShortCut('F');

	// Par defaut, on est en mode avec utilisation de la classe
	SetUsingClass(true);
}

KWDatabaseFormatDetectorView::~KWDatabaseFormatDetectorView() {}

void KWDatabaseFormatDetectorView::SetDatabase(KWDatabase* database)
{
	databaseFormatDetector.SetDatabase(database);
}

KWDatabase* KWDatabaseFormatDetectorView::GetDatabase() const
{
	return databaseFormatDetector.GetDatabase();
}

void KWDatabaseFormatDetectorView::SetUsingClass(boolean bValue)
{
	ALString sMessage;

	require(not IsOpened());
	databaseFormatDetector.SetUsingClass(bValue);

	// Parametrage de l'info-bulle selon le contexte
	sMessage = "Detect the format of the database.";
	sMessage += "\n Heuristic help that scans the first few lines to guess the file format";
	if (bValue)
		sMessage += "\n while being consistent w.r.t. the dictionary";
	else
		sMessage += ".";
	sMessage += "\n The header line and field separator are updated on success,";
	sMessage += "\n with a warning or an error in the log window only if necessary.";
	GetActionAt("DetectFileFormat")->SetHelpText(sMessage);
}

boolean KWDatabaseFormatDetectorView::GetUsingClass()
{
	return databaseFormatDetector.GetUsingClass();
}

void KWDatabaseFormatDetectorView::DetectFileFormat()
{
	databaseFormatDetector.DetectFileFormat();
}

const ALString KWDatabaseFormatDetectorView::GetClassLabel() const
{
	return databaseFormatDetector.GetClassLabel();
}

const ALString KWDatabaseFormatDetectorView::GetObjectLabel() const
{
	return databaseFormatDetector.GetObjectLabel();
}