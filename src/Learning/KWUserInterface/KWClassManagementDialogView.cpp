// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWClassManagementDialogView.h"

KWClassManagementDialogView::KWClassManagementDialogView()
{
	KWClassSpecArrayView* classSpecArrayView;
	KWClassSpecView* classSpecView;
	UIAction* inspectDictionaryAction;

	// Identifiant et libelle
	SetIdentifier("KWClassManagementDialog");
	SetLabel("Data dictionary");

	// Ajout d'un champ non editable pour le fichier dictionnaire, en tete de fenetre
	AddStringField("ClassFileName", "Dictionary file", "");
	GetFieldAt("ClassFileName")->SetEditable(false);
	MoveFieldBefore("ClassFileName", GetFieldAtIndex(0)->GetIdentifier());

	// La liste des dictionnaire de la classe mere est ici visible
	classSpecArrayView = cast(KWClassSpecArrayView*, GetFieldAt("Classes"));
	classSpecArrayView->SetVisible(true);

	// Parametrage de son action d'edition en menu popup
	inspectDictionaryAction = classSpecArrayView->GetActionAt("InspectItem");
	inspectDictionaryAction->SetVisible(true);
	inspectDictionaryAction->SetLabel("Inspect current dictionary");
	inspectDictionaryAction->SetStyle("PopupMenu");

	// On indique que les champs Used et Type des attributs sont editables dans
	// la fiche d'inspection d'un dictionnaire
	classSpecView = cast(KWClassSpecView*, classSpecArrayView->GetItemView());
	classSpecView->GetAttributeSpecArrayView()->GetFieldAt("Used")->SetEditable(true);
	classSpecView->GetAttributeSpecArrayView()->GetFieldAt("Type")->SetEditable(true);

	// Destruction des actions non utilisable de la classe mere
	DeleteActionAt("ManageClasses");
	DeleteActionAt("Quit");

	// Declaration des actions specifiques
	AddAction("BuildClassDefButton", "Build dictionary from data table...",
		  (ActionMethod)(&KWClassManagementDialogView::BuildClassDef));
	AddAction("EditFileButton", "Edit dictionary file", (ActionMethod)(&KWClassManagementDialogView::EditFile));

	// Le reload de la classe mere est accessible egalement par bouton
	AddAction("ReloadFileButton", "Reload dictionary file",
		  (ActionMethod)(&KWClassManagementActionView::ReloadFile));

	// Ajout d'un bouton pour les actions les plus importantes
	GetActionAt("BuildClassDefButton")->SetStyle("Button");
	GetActionAt("ReloadFileButton")->SetStyle("Button");
	GetActionAt("EditFileButton")->SetStyle("Button");

	// Info-bulles
	GetActionAt("BuildClassDefButton")
	    ->SetHelpText("Open a dialog box that allows to build dictionaries from data tables"
			  "\n then save them in a dictionary file.");
	GetActionAt("ReloadFileButton")->SetHelpText("Reload the current dictionary file into memory.");
	GetActionAt("EditFileButton")
	    ->SetHelpText("Edit the current dictionary file using the system text editor."
			  "\n To update the dictionary, save it from the text editor, then use the Reload button.");
	classSpecArrayView->GetActionAt("InspectItem")
	    ->SetHelpText(
		"Allow to inspect and partly modify a dictionary chosen among the list of available dictionaries."
		"\n The dictionary to inspect must be selected among the dictionaries in file.");

	// Short cuts
	GetActionAt("BuildClassDefButton")->SetShortCut('B');
	GetActionAt("ReloadFileButton")->SetShortCut('L');
	GetActionAt("EditFileButton")->SetShortCut('E');
}

KWClassManagementDialogView::~KWClassManagementDialogView() {}

KWClassManagement* KWClassManagementDialogView::GetKWClassManagement()
{
	require(objValue != NULL);
	return cast(KWClassManagement*, objValue);
}

void KWClassManagementDialogView::EventUpdate(Object* object)
{
	require(object != NULL);
}

void KWClassManagementDialogView::EventRefresh(Object* object)
{
	KWClassManagement* editedObject;

	require(object != NULL);

	editedObject = cast(KWClassManagement*, object);

	// Nom du fichier dictionnaire
	SetStringValueAt("ClassFileName", editedObject->GetClassFileName());

	// Reactualisation des specs de classes
	GetClassManagement()->RefreshClassSpecs();
}

void KWClassManagementDialogView::BuildClassDef()
{
	KWClassManagement* classManagement;
	ALString sDataPath;
	ALString sClassFileName;
	KWResultFilePathBuilder resultFilePathBuilder;

	// On remet le nom de la base a vide, pour ne garder que les specification de format
	// d'une utilisation a l'autre
	classBuilderView.GetSourceDataTable()->SetDatabaseName("");

	// Ouverture
	classBuilderView.Open();

	// Sauvegarde du fichier dictionnaire
	if (classBuilderView.GetBuildClassNumber() > 0)
	{
		// Acces a la gestion des dictionnaires
		classManagement = GetClassManagement();

		// On initialise le nom du dictionnaire a utiliser par defaut si necessaire
		sClassFileName = classManagement->GetClassFileName();
		if (sClassFileName == "")
			sClassFileName = classBuilderView.GetLastBuildClassName() + ".kdic";

		// Construction du chemin complet du dictionnaire a sauver
		resultFilePathBuilder.SetInputFilePathName(classBuilderView.GetSourceDataTable()->GetDatabaseName());
		resultFilePathBuilder.SetOutputFilePathName(sClassFileName);
		sClassFileName = resultFilePathBuilder.BuildResultFilePathName();

		// Sauvegarde du fichier par l'action standard "Enregistrer sous"
		SaveFileUnderName(sClassFileName);
		AddSimpleMessage("");

		// On initialise le choix du dictionnaire si necessaire
		if (classManagement->GetClassName() == "" or
		    KWClassDomain::GetCurrentDomain()->LookupClass(classManagement->GetClassName()) == NULL)
			classManagement->SetClassName(classManagement->SearchDefaultClassName());
	}
}

void KWClassManagementDialogView::EditFile()
{
	boolean bOk;
	char sResult[SYSTEM_MESSAGE_LENGTH + 1];

	if (GetClassManagement()->GetClassFileName() == "")
		AddWarning("No dictionary file available");
	else
	{
		bOk = OpenApplication("", "text editor", GetClassManagement()->GetClassFileName(), sResult);
		if (not bOk)
			AddWarning(sResult);
	}
}
