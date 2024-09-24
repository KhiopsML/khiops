// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWClassBuilderView.h"

KWClassBuilderView::KWClassBuilderView()
{
	KWSTDatabaseTextFileView* sourceDataTableView;
	ALString sStoredTypesLabel;
	int i;
	ALString sTmp;

	// Initialisation
	nBuildClassNumber = 0;
	sLastBuildClassName = "";

	// Parametrage general
	SetIdentifier("KWClassBuilderView");
	SetLabel("Dictionary builder");

	// Ajout de l'action de construction de dictionnaire
	AddAction("BuildClassDef", "Build dictionary from data table", (ActionMethod)(&KWClassBuilderView::BuildClass));
	GetActionAt("BuildClassDef")->SetStyle("Button");

	// Ajout du parametrage de la base d'origine
	sourceDataTableView = new KWSTDatabaseTextFileView;
	sourceDataTableView->SetObject(&sourceDataTable);
	AddCardField("SourceDataTable", "Input data table", sourceDataTableView);

	// Parametrage de la visibilite des specifications de la base d'origine
	sourceDataTableView->ToBasicReadMode();

	// Le detecteur de format est en mode sans utilisation de dictionnaire
	cast(KWDatabaseFormatDetectorView*, sourceDataTableView->GetDataView()->GetFieldAt("DatabaseFormatDetector"))
	    ->SetUsingClass(false);

	// Ajout de l'action de visualisation des premieres lignes
	sourceDataTableView->AddShowFirstLinesAction();

	// Creation d'une liste cachee des nom de classes en cours
	classNameList = new UIList;
	classNameList->AddStringField("Name", "Name", "");
	AddListField("Classes", "Existing dictionaries", classNameList);
	GetFieldAt("Classes")->SetEditable(false);

	// Liste des types stockes pour les info-bulles
	for (i = 0; i < KWType::None; i++)
	{
		if (KWType::IsStored(i))
		{
			if (sStoredTypesLabel != "")
				sStoredTypesLabel += ", ";
			sStoredTypesLabel += KWType::ToString(i);
		}
	}

	// Info-bulles
	sourceDataTableView->GetDataView()->GetFieldAt("DatabaseName")->SetHelpText("Name of the data table file");
	classNameList->GetFieldAt("Name")->SetHelpText("Name of dictionary.");
	GetActionAt("BuildClassDef")
	    ->SetHelpText(
		sTmp +
		"Start the analysis of the data table file to build a dictionary."
		"\n The first lines of the file are analyzed in order to determine the type of the variables:\n " +
		sStoredTypesLabel + "\n After analysis, the user can choose the name of the dictionary.");
	GetActionAt("Exit")->SetHelpText("Close the dialog box."
					 "\n If dictionaries have been built,"
					 "\n proposes to save them in a dictionary file.");

	// Short cuts
	GetActionAt("BuildClassDef")->SetShortCut('B');
}

KWClassBuilderView::~KWClassBuilderView() {}

KWSTDatabaseTextFile* KWClassBuilderView::GetSourceDataTable()
{
	return &sourceDataTable;
}

void KWClassBuilderView::Open()
{
	int nClass;
	KWClass* kwcClass;

	// Initialisation
	nBuildClassNumber = 0;
	sLastBuildClassName = "";

	// Reactualisation des specs de classes
	classNameList->RemoveAllItems();
	for (nClass = 0; nClass < KWClassDomain::GetCurrentDomain()->GetClassNumber(); nClass++)
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->GetClassAt(nClass);

		// Ajout du nom de la classe
		classNameList->InsertItemAt(classNameList->GetItemNumber());
		classNameList->SetStringValueAt("Name", kwcClass->GetName());
	}

	// Appel de la methode ancetre pour l'ouverture
	UICard::Open();
}

void KWClassBuilderView::BuildClass()
{
	boolean bOk = true;
	boolean bContinue;
	UIConfirmationCard classDefCard;
	ALString sClassName;
	KWClass* kwcClass;
	KWClass* kwcSimilarClass;

	// Verification de la presence d'un nom de base
	if (sourceDataTable.GetDatabaseName() == "")
	{
		AddError("Missing input database name");
		bOk = false;
	}

	// Verification uniquement du format de la base
	bOk = bOk and sourceDataTable.CheckFormat();

	// Construction du dictionnaire
	kwcClass = NULL;
	if (bOk)
	{
		// On initialise le nom a partir du prefix du fichier d'apprentissage
		sClassName = FileService::GetFilePrefix(sourceDataTable.GetDatabaseName());
		if (sClassName == "")
			sClassName = FileService::GetFileSuffix(sourceDataTable.GetDatabaseName());

		// On recherche un nom de classe nouveau
		sClassName = KWClassDomain::GetCurrentDomain()->BuildClassName(sClassName);

		// Demarage du suivi de la tache
		TaskProgression::SetTitle("Build dictionary fom data table");
		TaskProgression::Start();

		// Parametrage du driver la base source pour qu'il n'emette pas de warning pour des champs categoriels
		// trop long Cela permet d'identifier des champs Text via des champs categoriel
		KWDataTableDriverTextFile::SetOverlengthyFieldsVerboseMode(false);

		// Construction effective de la classe
		sourceDataTable.SetClassName(sClassName);
		kwcClass = sourceDataTable.ComputeClass();
		bOk = (kwcClass != NULL);

		// Restitutuion du parametrage initial du driver
		KWDataTableDriverTextFile::SetOverlengthyFieldsVerboseMode(true);

		// Fin du suivi de la tache
		TaskProgression::Stop();

		// Warning si dictionnaire existant de meme composition
		if (bOk)
		{
			assert(kwcClass != NULL);
			kwcSimilarClass = SearchSimilarClass(kwcClass);
			if (kwcSimilarClass != NULL)
			{
				AddWarning("New dictionary " + kwcClass->GetName() +
					   " has same composition than existing dictionary " +
					   kwcSimilarClass->GetName());
			}
		}
	}

	// Demande du nom du dictionnaire
	if (bOk)
	{
		// Parametrage de la boite d'ouverture
		classDefCard.SetIdentifier("KWBuildClassDefinition");
		classDefCard.SetLabel(GetLabel());
		classDefCard.GetActionAt("OK")->SetLabel("OK");
		classDefCard.GetActionAt("Exit")->SetLabel("Cancel");

		// Ajout d'un champ explicatif
		classDefCard.AddStringField("Info", "", "Enter a name to validate the dictionary");
		cast(UIStringElement*, classDefCard.GetFieldAt("Info"))->SetStyle("FormattedLabel");

		// Ajout d'un champ de saisie du nom du dictionnaire
		classDefCard.AddStringField("ClassName", "Dictionary name", "");
		cast(UIStringElement*, classDefCard.GetFieldAt("ClassName"))->SetMaxLength(KWClass::GetNameMaxLength());
		classDefCard.GetFieldAt("ClassName")->SetHelpText("Choose a name for the built dictionary.");

		// Parametrage du nom
		classDefCard.SetStringValueAt("ClassName", sClassName);

		// Ouverture de la boite de dialogue
		bContinue = true;
		while (bContinue)
		{
			bContinue = false;
			bOk = classDefCard.OpenAndConfirm();
			sClassName = classDefCard.GetStringValueAt("ClassName");

			// Test si le nom de classe est non vide
			if (bOk and sClassName == "")
			{
				bOk = false;
				AddError("Dictionary name is empty");
				bContinue = true;
			}

			// Test si le nom de classe est valide et different du nom par defaut
			if (bOk and sClassName != kwcClass->GetName())
			{
				assert(KWClass::CheckName(sClassName, KWClass::Class, NULL));
				bOk = (KWClassDomain::GetCurrentDomain()->LookupClass(sClassName) == NULL);
				if (not bOk)
				{
					AddError("Dictionary " + sClassName + " already exists");
					bContinue = true;
				}
			}
		}
	}

	// Prise en compte du nouveau dictionnaire
	if (bOk)
	{
		// Renommage eventuel du dictionnaire
		if (kwcClass->GetName() != sClassName)
			KWClassDomain::GetCurrentDomain()->RenameClass(kwcClass, sClassName);
		assert(kwcClass->GetName() == sClassName);

		// Ajout du nom de la classe dans la liste
		assert(kwcClass->GetName() == sClassName);
		classNameList->InsertItemAt(classNameList->GetItemNumber());
		classNameList->SetStringValueAt("Name", sClassName);

		// Mise a jour des stats
		nBuildClassNumber++;
		sLastBuildClassName = sClassName;
	}
	// Sinon, destruction si dictionnaire cree
	else if (kwcClass != NULL)
		KWClassDomain::GetCurrentDomain()->DeleteClass(kwcClass->GetName());
}

int KWClassBuilderView::GetBuildClassNumber() const
{
	return nBuildClassNumber;
}

const ALString& KWClassBuilderView::GetLastBuildClassName() const
{
	return sLastBuildClassName;
}

const ALString KWClassBuilderView::GetClassLabel() const
{
	return "Dictionary builder";
}

const ALString KWClassBuilderView::GetObjectLabel() const
{
	return sourceDataTable.GetDatabaseName();
}

KWClass* KWClassBuilderView::SearchSimilarClass(const KWClass* kwcSourceClass) const
{
	boolean bSimilar;
	int nClass;
	KWClass* kwcClass;
	KWAttribute* attribute;
	KWAttribute* sourceAttribute;

	require(kwcSourceClass != NULL);

	// Parcours des classes existantes
	bSimilar = false;
	for (nClass = 0; nClass < KWClassDomain::GetCurrentDomain()->GetClassNumber(); nClass++)
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->GetClassAt(nClass);

		// Test de similarite
		bSimilar = false;
		if (kwcClass != kwcSourceClass)
		{
			if (not bSimilar)
				bSimilar = kwcClass->GetAttributeNumber() == kwcSourceClass->GetAttributeNumber();
			if (bSimilar)
			{
				// Parcours des attributs
				attribute = kwcClass->GetHeadAttribute();
				while (attribute != NULL)
				{
					// Recherche d'un attribut de meme nom dans la classe source
					sourceAttribute = kwcSourceClass->LookupAttribute(attribute->GetName());

					// Test si non similaire
					if (sourceAttribute == NULL or
					    sourceAttribute->GetName() != attribute->GetName() or
					    sourceAttribute->GetType() != attribute->GetType())
						bSimilar = false;

					// Si similaire, test si attribut natif
					if (bSimilar and (sourceAttribute->GetAnyDerivationRule() != NULL or
							  attribute->GetAnyDerivationRule() != NULL))
						bSimilar = false;

					// Arret si pas similaire
					if (not bSimilar)
						break;

					// Attribut suivant
					kwcClass->GetNextAttribute(attribute);
				}
			}
		}

		// Arret avec warning si classe similaire
		if (bSimilar)
			return kwcClass;
	}
	return NULL;
}
