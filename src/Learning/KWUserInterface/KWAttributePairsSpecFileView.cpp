// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWAttributePairsSpecFileView.h"

KWAttributePairsSpecFileView::KWAttributePairsSpecFileView()
{
	// Libelles
	SetIdentifier("KWAttributePairsSpecFile");
	SetLabel("File");

	// Ajout des action
	AddAction("ImportPairs", "Import pairs...", (ActionMethod)(&KWAttributePairsSpecFileView::ImportPairs));
	AddAction("ExportPairs", "Export pairs...", (ActionMethod)(&KWAttributePairsSpecFileView::ExportPairs));

	// Info-bulles
	GetActionAt("ImportPairs")
	    ->SetHelpText("Import a list of variable pairs from a tabular text file with two columns.");
	GetActionAt("ExportPairs")
	    ->SetHelpText("Export the list of specific variable pairs to a tabular text file with two columns.");

	// Short cuts
	SetShortCut('F');
	GetActionAt("ImportPairs")->SetShortCut('I');
	GetActionAt("ExportPairs")->SetShortCut('E');
}

KWAttributePairsSpecFileView::~KWAttributePairsSpecFileView() {}

void KWAttributePairsSpecFileView::EventUpdate(Object* object) {}

void KWAttributePairsSpecFileView::EventRefresh(Object* object) {}

void KWAttributePairsSpecFileView::ImportPairs()
{
	UIFileChooserCard importCard;
	ALString sChosenFileName;

	// Ouverture du FileChooser
	sChosenFileName = importCard.ChooseFile("Import variable pairs", "Import", "FileChooser", "Data\ntxt",
						"ImportFileName", "Import file", "");

	// Ouverture du fichier des paires a importer
	if (sChosenFileName != "")
		GetAttributePairsSpec()->ImportAttributePairs(sChosenFileName);
}

void KWAttributePairsSpecFileView::ExportPairs()
{
	UIFileChooserCard exportCard;
	ALString sChosenFileName;

	// Ouverture du FileChooser
	sChosenFileName = exportCard.ChooseFile("Export variable pairs", "Export", "FileChooser", "Data\ntxt",
						"ExportFileName", "Export file", "");

	// Ouverture du fichier des paires a importer
	if (sChosenFileName != "")
		GetAttributePairsSpec()->ExportAllAttributePairs(sChosenFileName);
}

KWAttributePairsSpec* KWAttributePairsSpecFileView::GetAttributePairsSpec()
{
	require(objValue != NULL);

	return cast(KWAttributePairsSpec*, objValue);
}
