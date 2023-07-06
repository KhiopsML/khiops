// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWClassManagementDialogView.h"

KWClassManagementDialogView::KWClassManagementDialogView()
{
	KWClassSpecArrayView* classSpecArrayView;
	KWClassSpecView* classSpecView;

	// Identifiant et libelle
	SetIdentifier("KWClassManagementDialog");
	SetLabel("Dictionary management");

	// Ajout d'un champ non editable pour le fichier dictionnaire
	AddStringField("ClassFileName", "Dictionary file", "");
	GetFieldAt("ClassFileName")->SetEditable(false);

	// Declaration des actions
	AddAction("BuildClassDefButton", "Build dictionary from data table...",
		  (ActionMethod)(&KWClassManagementDialogView::BuildClassDef));
	AddAction("ReloadFileButton", "Reload dictionary file",
		  (ActionMethod)(&KWClassManagementDialogView::ReloadFile));
	AddAction("EditFileButton", "Edit dictionary file", (ActionMethod)(&KWClassManagementDialogView::EditFile));

	// Ajout d'un bouton pour les actions les plus importantes
	GetActionAt("BuildClassDefButton")->SetStyle("Button");
	GetActionAt("ReloadFileButton")->SetStyle("Button");
	GetActionAt("EditFileButton")->SetStyle("Button");

	// Ajout d'une donnee liste
	classSpecArrayView = new KWClassSpecArrayView;
	AddListField("Classes", "Dictionaries", classSpecArrayView);
	classSpecArrayView->SetEditable(false);
	classSpecArrayView->GetActionAt("InspectItem")->SetVisible(true);
	classSpecArrayView->GetActionAt("InspectItem")->SetLabel("Inspect current dictionary");
	classSpecArrayView->GetActionAt("InspectItem")->SetStyle("SmallButton");

	// On indique que les champs Used et Type des attributs sont editables dans
	// la fiche d'inspection d'un dictionnaire
	classSpecView = cast(KWClassSpecView*, classSpecArrayView->GetItemView());
	classSpecView->GetAttributeSpecArrayView()->GetFieldAt("Used")->SetEditable(true);
	classSpecView->GetAttributeSpecArrayView()->GetFieldAt("Type")->SetEditable(true);

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
	classSpecArrayView->SetShortCut('D');
	classSpecArrayView->GetActionAt("InspectItem")->SetShortCut('I');
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

const ALString KWClassManagementDialogView::GetClassLabel() const
{
	return "Class management";
}

const ALString KWClassManagementDialogView::GetObjectLabel() const
{
	// Redefini a vide, car le ClassLabel est suffisant dans les messages
	return "";
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

void KWClassManagementDialogView::ReloadFile()
{
	// On lit le dictionnaire
	if (GetClassManagement()->GetClassFileName() != "")
		GetClassManagement()->ReadClasses();
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

void KWClassManagementDialogView::SaveFileUnderName(const ALString& sFileName)
{
	UIFileChooserCard registerCard;
	ALString sChosenFileName;
	KWResultFilePathBuilder resultFilePathBuilder;

	// Ouverture du FileChooser
	sChosenFileName = registerCard.ChooseFile("Save as", "Save", "FileChooser", "Dictionary\nkdic", "ClassFileName",
						  "Dictionary file", sFileName);

	// Sauvegarde du fichier des dictionnaires
	if (sChosenFileName != "")
	{
		// Construction du chemin complet du dictionnaire a sauver
		resultFilePathBuilder.SetInputFilePathName(classBuilderView.GetSourceDataTable()->GetDatabaseName());
		resultFilePathBuilder.SetOutputFilePathName(sChosenFileName);
		sChosenFileName = resultFilePathBuilder.BuildResultFilePathName();

		// Memorisation du nom du fichier de dictionnaire
		GetClassManagement()->SetClassFileName(sChosenFileName);

		// On ecrit le fichier de dictionnaire
		GetClassManagement()->WriteClasses();
	}
}

void KWClassManagementDialogView::SetObject(Object* object)
{
	KWClassManagement* classManagement;

	require(object != NULL);

	// Acces a l'objet edite
	classManagement = cast(KWClassManagement*, object);

	// Parametrage de la sous-liste des specifications de dictionnaires
	cast(KWClassSpecArrayView*, GetFieldAt("Classes"))->SetObjectArray(classManagement->GetClassSpecs());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

KWClassManagement* KWClassManagementDialogView::GetClassManagement()
{
	require(objValue != NULL);

	return cast(KWClassManagement*, objValue);
}