// Copyright (c) 2023-2025 Orange. All rights reserved.
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

	// Warning si aucun predicteur interpretable trouve
	if (nPredictorNumber == 0)
	{
		Global::AddWarning("Interpret model", "", "No available classifier dictionary for interpretation");
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

	// Nettoyage de l'import si pas de predicteur specifie
	if (modelService->GetPredictorClassName() == "")
	{
		modelService->GetClassBuilder()->Clean();
		modelService->SetPredictorAttributeNumber(0);
	}
	// Import sinon, et cela doit marcher, selon les valeurs possible specifiees dans la methode Open
	else
	{
		// Recherche de la classe
		kwcPredictorClass =
		    KWClassDomain::GetCurrentDomain()->LookupClass(modelService->GetPredictorClassName());
		assert(kwcPredictorClass != NULL);

		// Import du predicteur
		modelService->GetClassBuilder()->ImportPredictor(kwcPredictorClass);
		modelService->SetPredictorAttributeNumber(
		    modelService->GetClassBuilder()->GetPredictorAttributeNumber());
	}
}

// ##
