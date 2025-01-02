// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWClassManagementView.h"

KWClassManagementView::KWClassManagementView()
{
	SetIdentifier("KWClassManagement");
	SetLabel("Dictionary management");
	AddStringField("ClassName", "Analysis dictionary", "");
	AddStringField("ClassFileName", "Dictionary file", "");

	// Parametrage des styles;
	GetFieldAt("ClassName")->SetStyle("EditableComboBox");

	// ## Custom constructor

	// Specialisation des extensions
	GetFieldAt("ClassFileName")->SetParameters("Dictionary\nkdic");

	// Le champs du fichier des dictionnaires est en read-only
	GetFieldAt("ClassFileName")->SetEditable(false);

	// Variables de travail
	KWClassSpecArrayView* classSpecArrayView;
	KWClassSpecView* classSpecView;

	// Declaration des actions
	AddAction("OpenFile", "Open...", (ActionMethod)(&KWClassManagementView::OpenFile));
	AddAction("CloseFile", "Close", (ActionMethod)(&KWClassManagementView::CloseFile));
	AddAction("SaveFile", "Save", (ActionMethod)(&KWClassManagementView::SaveFile));
	AddAction("SaveFileUnder", "Save as...", (ActionMethod)(&KWClassManagementView::SaveFileUnder));
	AddAction("ExportAsJSON", "Export as JSON...", (ActionMethod)(&KWClassManagementView::ExportAsJSON));
	AddAction("BuildClassDefButton", "Build dictionary from data table...",
		  (ActionMethod)(&KWClassManagementView::BuildClassDef));
	AddAction("ReloadFileButton", "Reload dictionary file", (ActionMethod)(&KWClassManagementView::ReloadFile));

	// Ajout d'accelerateurs sur les actions principales
	GetActionAt("OpenFile")->SetAccelKey("control O");
	GetActionAt("SaveFile")->SetAccelKey("control S");

	// Ajout d'un bouton pour les actions les plus importantes
	GetActionAt("ReloadFileButton")->SetStyle("Button");
	GetActionAt("BuildClassDefButton")->SetStyle("Button");

	// Ajout d'une donnee liste
	classSpecArrayView = new KWClassSpecArrayView;
	AddListField("Classes", "Dictionaries in file", classSpecArrayView);
	classSpecArrayView->SetEditable(false);
	classSpecArrayView->GetActionAt("InspectItem")->SetVisible(true);
	classSpecArrayView->GetActionAt("InspectItem")->SetLabel("Inspect current dictionary");

	// On indique que les champs Used et Type des attributs sont editables dans
	// la fiche d'inspection d'un dictionnaire
	classSpecView = cast(KWClassSpecView*, classSpecArrayView->GetItemView());
	classSpecView->GetAttributeSpecArrayView()->GetFieldAt("Used")->SetEditable(true);
	classSpecView->GetAttributeSpecArrayView()->GetFieldAt("Type")->SetEditable(true);

	// On indique que le champ de parametrage du dictionnaire declenche une action de rafraichissement
	// de l'interface immediatement apres une mise a jour, pour pouvoir rafraichir les mapping des databases
	cast(UIElement*, GetFieldAt("ClassName"))->SetTriggerRefresh(true);

	// Info-bulles
	GetActionAt("OpenFile")
	    ->SetHelpText("Open a dictionary file to load its dictionaries into memory"
			  "\n and make them available for data analysis.");
	GetActionAt("CloseFile")->SetHelpText("Remove the dictionaries from memory.");
	GetActionAt("SaveFile")->SetHelpText("Save the dictionaries in the current dictionary file.");
	GetActionAt("SaveFileUnder")->SetHelpText("Save the dictionaries in a new dictionary file.");
	GetActionAt("ExportAsJSON")->SetHelpText("Export the dictionaries under a JSON format.");
	GetActionAt("BuildClassDefButton")
	    ->SetHelpText("Open a dialog box that allows to build dictionaries from data tables"
			  "\n then save them in a dictionary file.");
	GetActionAt("ReloadFileButton")->SetHelpText("Reload the current dictionary file into memory.");
	classSpecArrayView->GetActionAt("InspectItem")
	    ->SetHelpText(
		"Allow to inspect and partly modify a dictionary chosen among the list of available dictionaries."
		"\n The dictionary to inspect must be selected among the dictionaries in file.");
	GetFieldAt("ClassName")->SetHelpText("Name of the dictionary related to the data to analyze.");
	GetFieldAt("ClassFileName")->SetHelpText("Name of the current dictionary file.");

	// Declaration d'un action de sortie globale de l'application dan sle menu dictionnaire
	AddAction("Quit", "Quit", (ActionMethod)(&KWClassManagementView::Quit));
	GetActionAt("Quit")->SetParameters("Exit");
	GetActionAt("Quit")->SetHelpText("Quit the application.");

	// Short cuts
	SetShortCut('D');
	GetActionAt("OpenFile")->SetShortCut('O');
	GetActionAt("CloseFile")->SetShortCut('C');
	GetActionAt("SaveFile")->SetShortCut('S');
	GetActionAt("SaveFileUnder")->SetShortCut('u');
	GetActionAt("ExportAsJSON")->SetShortCut('x');
	GetActionAt("BuildClassDefButton")->SetShortCut('B');
	GetActionAt("ReloadFileButton")->SetShortCut('L');
	GetActionAt("Quit")->SetShortCut('Q');
	classSpecArrayView->SetShortCut('D');
	classSpecArrayView->GetActionAt("InspectItem")->SetShortCut('I');

	// ##
}

KWClassManagementView::~KWClassManagementView()
{
	// ## Custom destructor

	// ##
}

KWClassManagement* KWClassManagementView::GetKWClassManagement()
{
	require(objValue != NULL);
	return cast(KWClassManagement*, objValue);
}

void KWClassManagementView::EventUpdate(Object* object)
{
	KWClassManagement* editedObject;

	require(object != NULL);

	editedObject = cast(KWClassManagement*, object);
	editedObject->SetClassName(GetStringValueAt("ClassName"));
	editedObject->SetClassFileName(GetStringValueAt("ClassFileName"));

	// ## Custom update

	// ##
}

void KWClassManagementView::EventRefresh(Object* object)
{
	KWClassManagement* editedObject;

	require(object != NULL);

	editedObject = cast(KWClassManagement*, object);
	SetStringValueAt("ClassName", editedObject->GetClassName());
	SetStringValueAt("ClassFileName", editedObject->GetClassFileName());

	// ## Custom refresh

	// Reactualisation des specs de classes
	GetClassManagement()->RefreshClassSpecs();

	// ##
}

const ALString KWClassManagementView::GetClassLabel() const
{
	return "Dictionary management";
}

// ## Method implementation

void KWClassManagementView::OpenFile()
{
	UIFileChooserCard openCard;
	boolean bOk;
	ALString sClassFileName;
	ALString sChosenFileName;

	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// Recherche de la derniere valeur utilisee
	sClassFileName = GetClassManagement()->GetClassFileName();

	// Ouverture du FileChooser
	sChosenFileName = openCard.ChooseFile("Open", "Open", "FileChooser", "Dictionary\nkdic", "ClassFileName",
					      "Dictionary file", sClassFileName);

	// Ouverture du fichier des dictionnaires
	if (sChosenFileName != "")
	{
		// Parametrage du nouveau nom du fichier de dictionnaire
		GetClassManagement()->SetClassFileName(sChosenFileName);

		// On lit le dictionnaire
		bOk = GetClassManagement()->ReadClasses();

		// On restitue l'ancien nom de fichier de dictionnaire si probleme
		if (not bOk)
			GetClassManagement()->SetClassFileName(sClassFileName);
	}
}

void KWClassManagementView::CloseFile()
{
	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// Supression des dictionnaires
	GetClassManagement()->DropClasses();

	// Nom du dictionnaire mis a vide
	GetClassManagement()->SetClassFileName("");
	GetClassManagement()->SetClassName("");
}

void KWClassManagementView::SaveFile()
{
	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// Sauvegarde
	if (GetClassManagement()->GetClassFileName() != "")
		GetClassManagement()->WriteClasses();
	else
		SaveFileUnder();
}

void KWClassManagementView::SaveFileUnder()
{
	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// Sauvegarde
	SaveFileUnderName(GetClassManagement()->GetClassFileName());
}

void KWClassManagementView::ExportAsJSON()
{
	ALString sJSONFileName;

	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// Sauvegarde
	sJSONFileName = GetClassManagement()->GetClassFileName();
	if (sJSONFileName != "")
		sJSONFileName = FileService::SetFileSuffix(sJSONFileName, "kdicj");
	ExportAsJSONFileUnderName(sJSONFileName);
}

void KWClassManagementView::BuildClassDef()
{
	KWClassManagement* classManagement;
	ALString sDataPath;
	ALString sClassFileName;

	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

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
		{
			sDataPath = FileService::GetPathName(classBuilderView.GetSourceDataTable()->GetDatabaseName());
			sClassFileName = FileService::BuildFilePathName(
			    sDataPath, classBuilderView.GetLastBuildClassName() + ".kdic");
		}

		// Sauvegarde du fichier par l'action standard "Enregistrer sous"
		SaveFileUnderName(sClassFileName);
		AddSimpleMessage("");

		// On initialise le choix du dictionnaire si necessaire
		if (classManagement->GetClassName() == "" or
		    KWClassDomain::GetCurrentDomain()->LookupClass(classManagement->GetClassName()) == NULL)
			classManagement->SetClassName(classManagement->SearchDefaultClassName());
	}
}

void KWClassManagementView::ReloadFile()
{
	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// On lit le dictionnaire
	if (GetClassManagement()->GetClassFileName() != "")
		GetClassManagement()->ReadClasses();
}

void KWClassManagementView::Quit() {}

void KWClassManagementView::SaveFileUnderName(const ALString& sFileName)
{
	UIFileChooserCard registerCard;
	ALString sChosenFileName;

	// Ouverture du FileChooser
	sChosenFileName = registerCard.ChooseFile("Save as", "Save", "FileChooser", "Dictionary\nkdic", "ClassFileName",
						  "Dictionary file", sFileName);

	// Sauvegarde du fichier des dictionnaires
	if (sChosenFileName != "")
	{
		// Memorisation du nom du fichier de dictionnaire
		GetClassManagement()->SetClassFileName(sChosenFileName);

		// On ecrit le fichier de dictionnaire
		GetClassManagement()->WriteClasses();
	}
}

void KWClassManagementView::ExportAsJSONFileUnderName(const ALString& sJSONFileName)
{
	UIFileChooserCard registerCard;
	ALString sChosenFileName;

	// Ouverture du FileChooser
	sChosenFileName = registerCard.ChooseFile("Export as JSON", "Export", "FileChooser", "JSON dictionary\nkdicj",
						  "JSONFileName", "JSON dictionary file", sJSONFileName);

	// Export vers un fichier JSON
	if (sChosenFileName != "")
	{
		// On ecrit le fichier de dictionnaire au format JSON
		GetClassManagement()->ExportJSONClasses(sChosenFileName);
	}
}

void KWClassManagementView::SetObject(Object* object)
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

KWClassManagement* KWClassManagementView::GetClassManagement()
{
	require(objValue != NULL);

	return cast(KWClassManagement*, objValue);
}

// ##
