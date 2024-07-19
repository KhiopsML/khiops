// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIPredictorInterpretationView.h"
#include "KWTrainedPredictor.h"

KIPredictorInterpretationView::KIPredictorInterpretationView()
{
	sourceDatabase = NULL;
	originDomain = NULL;
	interpretationSpec = new KIInterpretationSpec;

	// Parametrage general
	SetIdentifier("KIPredictorInterpretation");
	SetLabel("Interpret model");

	UIList* keyAttributeNameHelpList;

	SetIdentifier("KIInterpretationSpec");

	whyParameterView = new KIWhyParameterView;
	AddCardField("WhyParameter", "Variable importances", whyParameterView);

	howParameterView = new KIHowParameterView;
	AddCardField("HowParameter", "Reinforcement", howParameterView);

	// Passage en ergonomie onglets
	SetStyle("TabbedPanes");

	// Creation d'une liste cachee des attributs de la classe en cours
	keyAttributeNameHelpList = new UIList;
	keyAttributeNameHelpList->AddStringField("Name", "Name", "");
	AddListField("Attributes", "Variables", keyAttributeNameHelpList);
	keyAttributeNameHelpList->SetVisible(false);

	// Ajout de l'action de construction d'un dictionnaire enrichi
	AddAction("BuildInterpretationClass", "Build interpretation dictionary...",
		  (ActionMethod)(&KIPredictorInterpretationView::BuildInterpretationClass));
	GetActionAt("BuildInterpretationClass")->SetStyle("Button");

	// Ajout d'un champ de saisie du nom du dictionnaire
	AddStringField("ClassName", "Predictor dictionary", "");
	GetFieldAt("ClassName")->SetEditable(false);

	// Info-bulles
	GetFieldAt("ClassName")->SetHelpText("Name of the predictor dictionary");

	GetActionAt("BuildInterpretationClass")
	    ->SetHelpText("Build an interpretation dictionary that computes the variable importances and reinforcement "
			  "elements.");

	// Short cuts
	GetActionAt("BuildInterpretationClass")->SetShortCut('B');
}

KIPredictorInterpretationView::~KIPredictorInterpretationView()
{
	delete interpretationSpec;

	// remise a l'etat initial
	if (originDomain != NULL)
	{
		originDomain->Compile();
		KWClassDomain::SetCurrentDomain(originDomain);
	}
}

void KIPredictorInterpretationView::Open()
{
	KWClass* kwcClass;

	assert(interpretationSpec != NULL);

	// on travaille dans un domaine specifique a l'interpretation --> sauvegarder le domaine d'origine
	originDomain = KWClassDomain::GetCurrentDomain();

	if (sClassName == "")
	{
		Global::AddWarning("Interpret model", "", "No dictionary has been loaded for interpretation");
		GetActionAt("BuildInterpretationClass")->SetVisible(false);
	}
	else
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);
		if (not interpretationSpec->GetInterpretationDictionary()->ImportClassifier(kwcClass))
		{
			Global::AddWarning(
			    "Interpret model", "",
			    "The chosen dictionary is not a classifier that can be handled for interpretation");
			sClassName = "";
			GetActionAt("BuildInterpretationClass")->SetVisible(false);
		}
		else
		{
			SetStringValueAt("ClassName", sClassName);
			GetActionAt("BuildInterpretationClass")->SetVisible(true);

			KWClassDomain::SetCurrentDomain(
			    interpretationSpec->GetInterpretationDictionary()->GetInterpretationDomain());

			// initialiser la liste des variables leviers en fonction du dictionnaire d'interpretation que
			// l'on vient de generer
			interpretationSpec->GetLeverClassSpec()->SetClassName(
			    interpretationSpec->GetInterpretationDictionary()->GetInterpretationRootClass()->GetName());

			// remettre a Unused les attributs natifs du classifieur d'entree
			KWAttribute* attribute =
			    interpretationSpec->GetInterpretationDictionary()->GetInputClassifier()->GetHeadAttribute();
			while (attribute != NULL)
			{
				if (attribute->IsNative())
				{
					attribute->SetUsed(false);
					attribute->SetLoaded(false);
				}

				interpretationSpec->GetInterpretationDictionary()
				    ->GetInputClassifier()
				    ->GetNextAttribute(attribute);
			}
			interpretationSpec->GetInterpretationDictionary()->GetInterpretationDomain()->Compile();
		}
	}

	howParameterView->SetObject(interpretationSpec);
	whyParameterView->SetObject(interpretationSpec);

	// Appel de la methode ancetre pour l'ouverture
	UICard::Open();
}

void KIPredictorInterpretationView::InitializeSourceDatabase(KWDatabase* database,
							     const ALString& sDatabaseClassFileName)
{
	require(database != NULL);

	// Parametrage du nom du dictionnaire
	sClassFileName = sDatabaseClassFileName;
	sourceDatabase = database;
	sClassName = database->GetClassName();
	if (KWClassDomain::GetCurrentDomain()->LookupClass(sClassName) == NULL)
		sClassName = "";
}

void KIPredictorInterpretationView::BuildInterpretationClass()
{
	boolean bOk = true;
	KWClass* interpretationClass;
	ALString sTmp;
	UIFileChooserCard registerCard;
	ALString sChosenFileName;
	ALString sTargetPath;
	ALString sInterpretationClassFileName;
	ALString sOutputPathName;
	KWResultFilePathBuilder resultFilePathBuilder;

	assert(interpretationSpec != NULL);

	if (not interpretationSpec->Check())
		return;

	KIInterpretationDictionary* interpretationDictionary = interpretationSpec->GetInterpretationDictionary();

	if (interpretationDictionary->GetInputClassifier() == NULL)
	{
		AddWarning("No valid input classifier is available.");
		return;
	}

	// maj en fonction de l'IHM, des attributs d'interpretation, a partir d'un dictionnaire d'entree
	bOk = interpretationDictionary->UpdateInterpretationAttributes();

	sClassName = interpretationSpec->GetInterpretationDictionary()->GetInterpretationRootClass()->GetName();

	// La classe doit etre valide
	interpretationClass = NULL;
	if (sClassName != "")
		interpretationClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);
	if (bOk and interpretationClass == NULL)
	{
		bOk = false;
		AddError("Interpretation dictionary " + sClassName + " does not exist");
	}
	if (bOk and not interpretationClass->IsCompiled())
	{
		bOk = false;
		AddError("Incorrect interpretation dictionary " + sClassName);
	}

	// Ouverture d'une boite de dialogue pour le nom du fichier dictionnaire
	if (bOk)
	{
		// On initialise avec le nom du dictionnaire en prenant le chemin de la base source
		if (sourceDatabase->GetDatabaseName() != "")
			resultFilePathBuilder.SetInputFilePathName(sourceDatabase->GetDatabaseName());
		else if (sClassFileName != "")
			resultFilePathBuilder.SetInputFilePathName(sClassFileName);
		resultFilePathBuilder.SetOutputFilePathName("Interpretation.kdic");
		resultFilePathBuilder.SetFileSuffix("kdic");
		sInterpretationClassFileName = resultFilePathBuilder.BuildResultFilePathName();

		// Ouverture du FileChooser
		sChosenFileName =
		    registerCard.ChooseFile("Save as", "Save", "FileChooser", "Dictionary\nkdic", "ClassFileName",
					    "Interpretation dictionary file", sInterpretationClassFileName);

		// Verification du nom du fichier de dictionnaire
		if (sChosenFileName != "")
		{
			// Construction du chemin complet du dictionnaire a sauver
			resultFilePathBuilder.SetOutputFilePathName(sChosenFileName);
			sInterpretationClassFileName = resultFilePathBuilder.BuildResultFilePathName();

			// generation du dictionaire si OK
			if (bOk)
			{
				interpretationDictionary->GetInterpretationDomain()->WriteFile(
				    sInterpretationClassFileName);

				if (interpretationSpec->GetWhyAttributesNumber() == 0)
					Global::AddWarning(
					    "", "",
					    "The maximum number of variables in the contribution analysis is set to 0 "
					    ": no contribution analysis will be done.");

				if (interpretationSpec->GetHowAttributesNumber() == 0)
					Global::AddWarning(
					    "", "",
					    "The maximum number of variables in the reinforcement analysis is set to 0 "
					    ": no reinforcement analysis will be done.");
				else if (interpretationSpec->GetHowClass() ==
					 KIInterpretationDictionary::NO_VALUE_LABEL)
					Global::AddWarning("", "",
							   "No reinforcement class has been specified : no "
							   "reinforcement analysis will be done.");

				AddSimpleMessage("Write interpretation dictionary file " +
						 sInterpretationClassFileName);
				AddSimpleMessage("");
			}
		}
	}
}

const ALString KIPredictorInterpretationView::GetClassLabel() const
{
	return "Interpret model";
}
