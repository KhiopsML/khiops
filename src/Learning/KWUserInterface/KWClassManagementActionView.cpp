// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWClassManagementActionView.h"

KWClassManagementActionView::KWClassManagementActionView()
{
	SetIdentifier("KWClassManagementAction");
	SetLabel("Dictionary management");

	// Ajout d'une donnee liste pour obtenir de l'aide sur les dictionnaires charges en memoire
	// depuis les autres panneaux de l'interface
	AddListField("Classes", "Dictionaries", new KWClassSpecArrayView);
	GetFieldAt("Classes")->SetEditable(false);
	GetFieldAt("Classes")->SetVisible(false);

	// Declaration des actions
	AddAction("OpenFile", "Open...", (ActionMethod)(&KWClassManagementActionView::OpenFile));
	AddAction("CloseFile", "Close", (ActionMethod)(&KWClassManagementActionView::CloseFile));
	AddAction("ReloadFile", "Reload", (ActionMethod)(&KWClassManagementActionView::ReloadFile));
	AddAction("SaveFile", "Save", (ActionMethod)(&KWClassManagementActionView::SaveFile));
	AddAction("SaveFileUnder", "Save as...", (ActionMethod)(&KWClassManagementActionView::SaveFileUnder));
	AddAction("ExportAsJSON", "Export as JSON...", (ActionMethod)(&KWClassManagementActionView::ExportAsJSON));
	AddAction("ManageClasses", "Manage dictionaries...",
		  (ActionMethod)(&KWClassManagementActionView::ManageClasses));

	// Ajout d'accelerateurs sur les actions principales
	GetActionAt("OpenFile")->SetAccelKey("control O");
	GetActionAt("ReloadFile")->SetAccelKey("control R");
	GetActionAt("SaveFile")->SetAccelKey("control S");
	GetActionAt("ManageClasses")->SetAccelKey("control M");

	// Info-bulles
	GetActionAt("OpenFile")
	    ->SetHelpText("Open a dictionary file to load its dictionaries into memory"
			  "\n and make them available for data analysis.");
	GetActionAt("CloseFile")->SetHelpText("Remove the dictionaries from memory.");
	GetActionAt("ReloadFile")->SetHelpText("Reload the current dictionary file into memory.");
	GetActionAt("SaveFile")->SetHelpText("Save the dictionaries in the current dictionary file.");
	GetActionAt("SaveFileUnder")->SetHelpText("Save the dictionaries in a new dictionary file.");
	GetActionAt("ExportAsJSON")->SetHelpText("Export the dictionaries under a JSON format.");
	GetActionAt("ManageClasses")->SetHelpText("Open a dialog box that allows to build and modify dictionaries.");

	// Declaration d'un action de sortie globale de l'application dans le menu dictionnaire
	AddAction("Quit", "Quit", (ActionMethod)(&KWClassManagementActionView::Quit));
	GetActionAt("Quit")->SetParameters("Exit");
	GetActionAt("Quit")->SetHelpText("Quit the application.");

	// Short cuts
	SetShortCut('D');
	GetActionAt("OpenFile")->SetShortCut('O');
	GetActionAt("CloseFile")->SetShortCut('C');
	GetActionAt("SaveFile")->SetShortCut('S');
	GetActionAt("SaveFileUnder")->SetShortCut('u');
	GetActionAt("ExportAsJSON")->SetShortCut('x');
	GetActionAt("ManageClasses")->SetShortCut('M');
	GetActionAt("Quit")->SetShortCut('Q');

	// Initialisation des bases d'apprentissage et test
	trainDatabase = NULL;
	testDatabase = NULL;
}

KWClassManagementActionView::~KWClassManagementActionView() {}

KWClassManagement* KWClassManagementActionView::GetKWClassManagement()
{
	require(objValue != NULL);
	return cast(KWClassManagement*, objValue);
}

void KWClassManagementActionView::EventUpdate(Object* object)
{
	require(object != NULL);
}

void KWClassManagementActionView::EventRefresh(Object* object)
{
	require(object != NULL);

	// Reactualisation des specs de classes
	GetClassManagement()->RefreshClassSpecs();
}

const ALString KWClassManagementActionView::GetClassLabel() const
{
	return "Dictionary management";
}

void KWClassManagementActionView::OpenFile()
{
	UIFileChooserCard openCard;
	boolean bOk;
	ALString sClassFileName;
	ALString sChosenFileName;

	require(CheckDictionaryName());

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

	// Copie du nom du dictionnaire dans les bases
	CopyDictionaryNameToDatabases();
}

void KWClassManagementActionView::CloseFile()
{
	require(CheckDictionaryName());

	// Supression des dictionnaires
	GetClassManagement()->DropClasses();

	// Nom du dictionnaire mis a vide
	GetClassManagement()->SetClassFileName("");
	GetClassManagement()->SetClassName("");

	// Copie du nom du dictionnaire dans les bases
	CopyDictionaryNameToDatabases();
}

void KWClassManagementActionView::ReloadFile()
{
	require(CheckDictionaryName());

	// On lit le dictionnaire
	if (GetClassManagement()->GetClassFileName() != "")
		GetClassManagement()->ReadClasses();
}

void KWClassManagementActionView::SaveFile()
{
	require(CheckDictionaryName());

	// Sauvegarde
	if (GetClassManagement()->GetClassFileName() != "")
		GetClassManagement()->WriteClasses();
	else
		SaveFileUnder();

	// Copie du nom du dictionnaire dans les bases
	CopyDictionaryNameToDatabases();
}

void KWClassManagementActionView::SaveFileUnder()
{
	require(CheckDictionaryName());

	// Sauvegarde
	SaveFileUnderName(GetClassManagement()->GetClassFileName());

	// Copie du nom du dictionnaire dans les bases
	CopyDictionaryNameToDatabases();
}

void KWClassManagementActionView::ExportAsJSON()
{
	ALString sJSONFileName;

	require(CheckDictionaryName());

	// Sauvegarde
	sJSONFileName = GetClassManagement()->GetClassFileName();
	if (sJSONFileName != "")
		sJSONFileName = FileService::SetFileSuffix(sJSONFileName, "kdicj");
	ExportAsJSONFileUnderName(sJSONFileName);

	// Copie du nom du dictionnaire dans les bases
	CopyDictionaryNameToDatabases();
}

void KWClassManagementActionView::ManageClasses()
{
	KWClassManagementDialogView manageClassesDialogView;

	require(CheckDictionaryName());

	// Ouverture de la boite avec l'objet edite en cours
	// On prend le RawObject pour avoir l'objet directement, sans synchronisation avec l'interface
	manageClassesDialogView.SetObject(GetRawObject());
	manageClassesDialogView.Open();

	// Copie du nom du dictionnaire dans les bases
	CopyDictionaryNameToDatabases();
}

void KWClassManagementActionView::Quit() {}

void KWClassManagementActionView::SaveFileUnderName(const ALString& sFileName)
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
		// Construction du chemin complet en sortie
		resultFilePathBuilder.SetInputFilePathName(GetClassManagement()->GetClassFileName());
		resultFilePathBuilder.SetOutputFilePathName(sChosenFileName);
		resultFilePathBuilder.SetFileSuffix("kdic");
		sChosenFileName = resultFilePathBuilder.BuildResultFilePathName();

		// Memorisation du nom du fichier de dictionnaire
		GetClassManagement()->SetClassFileName(sChosenFileName);

		// On ecrit le fichier de dictionnaire
		GetClassManagement()->WriteClasses();
	}
}

void KWClassManagementActionView::ExportAsJSONFileUnderName(const ALString& sJSONFileName)
{
	UIFileChooserCard registerCard;
	ALString sChosenFileName;
	KWResultFilePathBuilder resultFilePathBuilder;

	// Ouverture du FileChooser
	sChosenFileName = registerCard.ChooseFile("Export as JSON", "Export", "FileChooser", "JSON dictionary\nkdicj",
						  "JSONFileName", "JSON dictionary file", sJSONFileName);

	// Export vers un fichier JSON
	if (sChosenFileName != "")
	{
		// Construction du chemin complet en sortie
		resultFilePathBuilder.SetInputFilePathName(GetClassManagement()->GetClassFileName());
		resultFilePathBuilder.SetOutputFilePathName(sChosenFileName);
		resultFilePathBuilder.SetFileSuffix("kdicj");
		sChosenFileName = resultFilePathBuilder.BuildResultFilePathName();

		// On ecrit le fichier de dictionnaire au format JSON
		GetClassManagement()->ExportJSONClasses(sChosenFileName);
	}
}

void KWClassManagementActionView::SetTrainDatabase(KWDatabase* trainDatabaseSettings)
{
	trainDatabase = trainDatabaseSettings;
}

KWDatabase* KWClassManagementActionView::GetTrainDatabase()
{
	return trainDatabase;
}

void KWClassManagementActionView::SetTestDatabase(KWDatabase* testDatabaseSettings)
{
	testDatabase = testDatabaseSettings;
}

KWDatabase* KWClassManagementActionView::GetTestDatabase()
{
	return testDatabase;
}

void KWClassManagementActionView::SetObject(Object* object)
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

KWClassManagement* KWClassManagementActionView::GetClassManagement()
{
	require(objValue != NULL);

	return cast(KWClassManagement*, objValue);
}

boolean KWClassManagementActionView::CheckDictionaryName()
{
	boolean bOk = true;

	// On verifie la coherence du nom du dictionnaire par defaut avec les bases de train et test
	bOk = bOk and (testDatabase == NULL or trainDatabase == NULL or
		       testDatabase->GetClassName() == trainDatabase->GetClassName());
	bOk = bOk and (testDatabase == NULL or trainDatabase == NULL or
		       GetClassManagement()->GetClassName() == trainDatabase->GetClassName());
	return bOk;
}

void KWClassManagementActionView::CopyDictionaryNameToDatabases()
{
	// On recopie le nom du dictionnaire par defaut dans les bases de train et test
	if (trainDatabase != NULL)
		trainDatabase->SetClassName(GetClassManagement()->GetClassName());
	if (testDatabase != NULL)
		testDatabase->SetClassName(GetClassManagement()->GetClassName());
}
