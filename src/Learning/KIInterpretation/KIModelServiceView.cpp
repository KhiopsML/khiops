// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIModelServiceView.h"

KIModelServiceView::KIModelServiceView()
{
	SetIdentifier("KIModelService");
	SetLabel("Interpretation service");
	AddStringField("PredictorClassName", "Predictor dictionary", "");
	AddIntField("PredictorAttributeNumber", "Number of predictor variables", 0);

	// ## Custom constructor

	// Base d'apprentissage
	trainDatabase = NULL;

	// Le nombre d'attributs du predicteur est en read-only
	GetFieldAt("PredictorAttributeNumber")->SetEditable(false);

	// On indique que le champ de parametrage du dictionnaire declenche une action de rafraichissement
	// de l'interface immediatement apres une mise a jour, pour pouvoir rafraichir les mapping des databases
	cast(UIElement*, GetFieldAt("PredictorClassName"))->SetTriggerRefresh(true);

	// Info-bulles
	GetFieldAt("PredictorClassName")->SetHelpText("Name of the predictor dictionary");
	GetFieldAt("PredictorAttributeNumber")->SetHelpText("Number of variables used by the predictor");

	// ##
}

KIModelServiceView::~KIModelServiceView()
{
	// ## Custom destructor

	// ##
}

KIModelService* KIModelServiceView::GetKIModelService()
{
	require(objValue != NULL);
	return cast(KIModelService*, objValue);
}

void KIModelServiceView::EventUpdate(Object* object)
{
	KIModelService* editedObject;

	require(object != NULL);

	editedObject = cast(KIModelService*, object);
	editedObject->SetPredictorClassName(GetStringValueAt("PredictorClassName"));
	editedObject->SetPredictorAttributeNumber(GetIntValueAt("PredictorAttributeNumber"));

	// ## Custom update

	// ##
}

void KIModelServiceView::EventRefresh(Object* object)
{
	KIModelService* editedObject;

	require(object != NULL);

	editedObject = cast(KIModelService*, object);
	SetStringValueAt("PredictorClassName", editedObject->GetPredictorClassName());
	SetIntValueAt("PredictorAttributeNumber", editedObject->GetPredictorAttributeNumber());

	// ## Custom refresh

	// Variables locales
	ALString sImportedPredictorClassName;

	// Recherche du predicteur courant importe
	if (editedObject->GetClassBuilder()->IsPredictorImported())
		sImportedPredictorClassName = editedObject->GetClassBuilder()->GetPredictorClass()->GetName();

	// Reactualisation de l'import en cas de changement
	if (editedObject->GetPredictorClassName() != sImportedPredictorClassName)
	{
		// Rafraichissement des specifications du predicteur
		RefreshPredictorSpec(editedObject);

		// Mise a jour du nombre d'attributs du predicteur
		SetIntValueAt("PredictorAttributeNumber", editedObject->GetPredictorAttributeNumber());
	}

	// ##
}

const ALString KIModelServiceView::GetClassLabel() const
{
	return "Interpretation service";
}

// ## Method implementation

void KIModelServiceView::SetDefaultClassName(const ALString& sValue)
{
	sDefaultClassName = sValue;
}

const ALString& KIModelServiceView::GetDefaultClassName() const
{
	return sDefaultClassName;
}

void KIModelServiceView::SetTrainDatabase(const KWDatabase* database)
{
	trainDatabase = database;
}

const KWDatabase* KIModelServiceView::GetTrainDatabase() const
{
	return trainDatabase;
}

void KIModelServiceView::SetClassFileName(const ALString& sValue)
{
	sClassFileName = sValue;
}

const ALString& KIModelServiceView::GetClassFileName() const
{
	return sClassFileName;
}

ALString KIModelServiceView::ChooseDictionaryFileName(const ALString& sDictionaryPrefix)
{
	boolean bOk = true;
	ALString sDictionaryFileName;
	KWResultFilePathBuilder resultFilePathBuilder;
	UIFileChooserCard registerCard;
	ObjectArray oaTrainDatabaseFileSpecs;
	int nRef;
	FileSpec* specRef;
	FileSpec specInputDictionaryFile;
	FileSpec specDictionaryFile;

	// Construction du nom du fichier de dictionnaire en utilisant si possible les repertoires disponibles
	if (GetTrainDatabase() != NULL and GetTrainDatabase()->GetDatabaseName() != "")
		resultFilePathBuilder.SetInputFilePathName(GetTrainDatabase()->GetDatabaseName());
	else if (GetClassFileName() != "")
		resultFilePathBuilder.SetInputFilePathName(GetClassFileName());
	else
		resultFilePathBuilder.SetInputFilePathName(".");
	resultFilePathBuilder.SetOutputFilePathName(sDictionaryPrefix + ".kdic");
	resultFilePathBuilder.SetFileSuffix("kdic");
	sDictionaryFileName = resultFilePathBuilder.BuildResultFilePathName();

	// Ouverture du FileChooser pour obtenir le nom du fichier a transfere, ou vide si annulation
	sDictionaryFileName =
	    registerCard.ChooseFile("Save as", "Save", "FileChooser", "Dictionary\nkdic", "ClassFileName",
				    sDictionaryPrefix + " dictionary file", sDictionaryFileName);
	bOk = (sDictionaryFileName != "");

	// Verification du nom du fichier de dictionnaire
	if (bOk)
	{
		// Construction du chemin complet du dictionnaire a sauver
		resultFilePathBuilder.SetOutputFilePathName(sDictionaryFileName);
		sDictionaryFileName = resultFilePathBuilder.BuildResultFilePathName();

		// Specification du fichier dictionnaire en sortie
		specDictionaryFile.SetLabel(sDictionaryPrefix + " dictionary");
		specDictionaryFile.SetFilePathName(sDictionaryFileName);

		// Test de non collision avec le fichier de dictionnaire en entree
		if (bOk and GetClassFileName() != "")
		{
			// Specification du fichier dictionnaire en entree
			specInputDictionaryFile.SetLabel("input dictionary");
			specInputDictionaryFile.SetFilePathName(GetClassFileName());

			// Verification de non collision
			bOk = bOk and specDictionaryFile.CheckReferenceFileSpec(&specInputDictionaryFile);
		}

		// Test de non collision avec des fichiers de la base source
		if (bOk and GetTrainDatabase() != NULL)
		{
			GetTrainDatabase()->ExportUsedFileSpecs(&oaTrainDatabaseFileSpecs);
			for (nRef = 0; nRef < oaTrainDatabaseFileSpecs.GetSize(); nRef++)
			{
				specRef = cast(FileSpec*, oaTrainDatabaseFileSpecs.GetAt(nRef));
				specRef->SetLabel("source " + specRef->GetLabel());
				bOk = bOk and specDictionaryFile.CheckReferenceFileSpec(specRef);
				if (not bOk)
					break;
			}
			oaTrainDatabaseFileSpecs.DeleteAll();
		}

		// On annule le choix du dictionnaire en cas d'erreur
		if (not bOk)
			sDictionaryFileName = "";
	}
	ensure(not bOk or sDictionaryFileName != "");
	return sDictionaryFileName;
}

void KIModelServiceView::Open()
{
	KWClass* kwcClass;
	int nClass;
	ALString sFirstPredictorClassName;
	ALString sPredictorClassNames;
	int nPredictorNumber;

	// Parcours des dictionnaire courant pour identifier les predicteurs interpretables
	nPredictorNumber = 0;
	for (nClass = 0; nClass < KWClassDomain::GetCurrentDomain()->GetClassNumber(); nClass++)
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->GetClassAt(nClass);

		// On cherche a s'avoir si le dictionnaire correspond a un predicteur interpretable
		if (KWType::IsSimple(KWTrainedPredictor::GetMetaDataPredictorType(kwcClass)))
		{
			if (GetKIModelService()->GetClassBuilder()->ImportPredictor(kwcClass))
			{
				// Memorisation du premier predicteur
				if (nPredictorNumber == 0)
					sFirstPredictorClassName = kwcClass->GetName();

				// On choisit le predicteur si'il correspond au dictionnaire par defaut
				if (kwcClass->GetName() == GetDefaultClassName())
					sFirstPredictorClassName = kwcClass->GetName();

				// Memorisation de la liste de tous les predicteurs disponibles
				if (nPredictorNumber > 0)
					sPredictorClassNames += "\n";
				sPredictorClassNames += kwcClass->GetName();
				nPredictorNumber++;
			}
		}
	}

	// Warning si aucun predicteur interpretable trouve
	if (nPredictorNumber == 0)
	{
		Global::AddWarning("Interpret model", "",
				   "No available classifier dictionary for interpretation services");
	}
	else
	{
		// Parametrage du champ de saisie des dictionnaires en style ComboBox,
		// avec la liste des dictionnaires en cours
		GetFieldAt("PredictorClassName")->SetStyle("EditableComboBox");
		GetFieldAt("PredictorClassName")->SetParameters(sPredictorClassNames);

		// Parametrage du predicteur par defaut
		GetKIModelService()->SetPredictorClassName(sFirstPredictorClassName);
		RefreshPredictorSpec(GetKIModelService());
	}

	// Appel de la methode ancetre pour l'ouverture
	UICard::Open();
}

void KIModelServiceView::RefreshPredictorSpec(KIModelService* modelService)
{
	KWClass* kwcPredictorClass;

	require(modelService != NULL);

	// Nettoyage prealable
	modelService->GetClassBuilder()->Clean();
	modelService->SetPredictorAttributeNumber(0);

	// Import selon le nom du preducteur choisi
	if (modelService->GetPredictorClassName() != "")
	{
		// Recherche de la classe
		kwcPredictorClass =
		    KWClassDomain::GetCurrentDomain()->LookupClass(modelService->GetPredictorClassName());

		// Import du predicteur si la classe correspondante existe
		if (kwcPredictorClass != NULL)
		{
			modelService->GetClassBuilder()->ImportPredictor(kwcPredictorClass);

			// Memorisation du nombre d'attribut si possible
			if (modelService->GetClassBuilder()->IsPredictorImported())
				modelService->SetPredictorAttributeNumber(
				    modelService->GetClassBuilder()->GetPredictorAttributeNumber());
		}
	}
}

// ##
