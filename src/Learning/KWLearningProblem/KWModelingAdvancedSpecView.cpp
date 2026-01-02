// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWModelingAdvancedSpecView.h"

KWModelingAdvancedSpecView::KWModelingAdvancedSpecView()
{
	SetIdentifier("KWModelingAdvancedSpec");
	SetLabel("Advanced predictor parameters");
	AddBooleanField("DataPreparationOnly", "Do data preparation only", false);
	AddBooleanField("InterpretableNames", "Build interpretable names", false);

	// Parametrage des styles;
	GetFieldAt("DataPreparationOnly")->SetStyle("CheckBox");
	GetFieldAt("InterpretableNames")->SetStyle("CheckBox");

	// ## Custom constructor

	// Ajout d'une sous-fiche pour les parametres du SNB
	AddCardField("SelectiveNaiveBayesParameters", "Selective Naive Bayes parameters",
		     new SNBPredictorSelectiveNaiveBayesView);

	// Parametrage de nom interpretables apres les parametre du SNB
	// Uniquement en mode expert
	MoveFieldBefore("SelectiveNaiveBayesParameters", "InterpretableNames");
	GetFieldAt("InterpretableNames")->SetVisible(GetLearningExpertMode());

	// Declaration des actions
	// On utilise exceptionnellement un format html pour le libelle des actions,
	// pour l'avoir centre et sur deux lignes
	AddAction("InspectConstructionDomain",
		  "<html> <center> Variable construction <br> parameters </center> </html>",
		  (ActionMethod)(&KWModelingAdvancedSpecView::InspectConstructionDomain));
	AddAction("InspectTextFeaturesParameters", "<html> <center> Text feature <br> parameters </center> </html>",
		  (ActionMethod)(&KWModelingAdvancedSpecView::InspectTextFeaturesParameters));
	AddAction("InspectAttributePairsParameters", "<html> <center> Variable pairs <br> parameters </center> </html>",
		  (ActionMethod)(&KWModelingAdvancedSpecView::InspectAttributePairsParameters));
	AddAction("InspectAttributeCreationParameters",
		  "<html> <center> Tree construction <br> parameters </center> </html>",
		  (ActionMethod)(&KWModelingAdvancedSpecView::InspectAttributeCreationParameters));
	GetActionAt("InspectConstructionDomain")->SetStyle("Button");
	GetActionAt("InspectTextFeaturesParameters")->SetStyle("Button");
	GetActionAt("InspectAttributePairsParameters")->SetStyle("Button");
	GetActionAt("InspectAttributeCreationParameters")->SetStyle("Button");

	// Action d'edition des parametre des arbres disponible uniquement en mode avance
	GetActionAt("InspectAttributeCreationParameters")
	    ->SetVisible(KDDataPreparationAttributeCreationTask::GetGlobalCreationTask() != NULL and
			 KDDataPreparationAttributeCreationTaskView::GetGlobalCreationTaskView() != NULL);

	// Info-bulles
	GetFieldAt("DataPreparationOnly")
	    ->SetHelpText("Do the data preparation step only."
			  "\n Do not perform the modeling step in supervised analysis.");
	GetFieldAt("InterpretableNames")
	    ->SetHelpText("Build interpretable names for the automatically multi-table and text constructed variables."
			  "\n Expert mode only"
			  "\n Non-interpretable names based solely on alphanumeric characters may be required "
			  "\n when data is to be exported to databases with column naming constraints."
			  "\n Please note: only multi-table and text features are supported, and the analysis results"
			  "\n for pairs and trees may vary when this option is activated");
	GetActionAt("InspectConstructionDomain")
	    ->SetHelpText(
		"Advanced parameters to select the construction rules used for automatic variable construction.");
	GetActionAt("InspectTextFeaturesParameters")
	    ->SetHelpText("Advanced parameters for the construction of text features.");
	GetActionAt("InspectAttributePairsParameters")
	    ->SetHelpText("Advanced parameters to select the variable pairs to analyze.");
	GetActionAt("InspectAttributeCreationParameters")
	    ->SetHelpText("Advanced parameters for the constuction of tree based variables.");

	// ##
}

KWModelingAdvancedSpecView::~KWModelingAdvancedSpecView()
{
	// ## Custom destructor

	// ##
}

KWModelingSpec* KWModelingAdvancedSpecView::GetKWModelingSpec()
{
	require(objValue != NULL);
	return cast(KWModelingSpec*, objValue);
}

void KWModelingAdvancedSpecView::EventUpdate(Object* object)
{
	KWModelingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWModelingSpec*, object);
	editedObject->SetDataPreparationOnly(GetBooleanValueAt("DataPreparationOnly"));
	editedObject->SetInterpretableNames(GetBooleanValueAt("InterpretableNames"));

	// ## Custom update

	// ##
}

void KWModelingAdvancedSpecView::EventRefresh(Object* object)
{
	KWModelingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWModelingSpec*, object);
	SetBooleanValueAt("DataPreparationOnly", editedObject->GetDataPreparationOnly());
	SetBooleanValueAt("InterpretableNames", editedObject->GetInterpretableNames());

	// ## Custom refresh

	// ##
}

const ALString KWModelingAdvancedSpecView::GetClassLabel() const
{
	return "Advanced predictor parameters";
}

// ## Method implementation

void KWModelingAdvancedSpecView::InspectConstructionDomain()
{
	KWModelingSpec* modelingSpec;
	KDConstructionDomainView constructionDomainView;

	// Acces a l'objet edite
	modelingSpec = cast(KWModelingSpec*, GetObject());
	check(modelingSpec);

	// Ouverture de la sous-fiche
	constructionDomainView.SetObject(modelingSpec->GetAttributeConstructionSpec()->GetConstructionDomain());
	constructionDomainView.Open();
}

void KWModelingAdvancedSpecView::InspectTextFeaturesParameters()
{
	KWModelingSpec* modelingSpec;
	KDTextFeatureSpecView textFeatureSpecView;

	// Acces a l'objet edite
	modelingSpec = cast(KWModelingSpec*, GetObject());
	check(modelingSpec);

	// Ouverture de la sous-fiche
	textFeatureSpecView.SetObject(modelingSpec->GetAttributeConstructionSpec()->GetTextFeatureSpec());
	textFeatureSpecView.Open();
}

void KWModelingAdvancedSpecView::InspectAttributeCreationParameters()
{
	KWModelingSpec* modelingSpec;
	KDDataPreparationAttributeCreationTaskView* attributeCreationView;

	// Acces a l'objet edite
	modelingSpec = cast(KWModelingSpec*, GetObject());
	check(modelingSpec);

	// Message si pas d'objet edite
	if (modelingSpec->GetAttributeConstructionSpec()->GetAttributeCreationParameters() == NULL)
		AddMessage("No parameter available");
	// Message si pas de vue d'edition des parametres
	else if (KDDataPreparationAttributeCreationTaskView::GetGlobalCreationTaskView() == NULL)
		AddMessage("No view available to update parameters");
	// Sinon, edition de l'objet
	else
	{
		attributeCreationView = KDDataPreparationAttributeCreationTaskView::GetGlobalCreationTaskView();

		// Ouverture de la sous-fiche
		attributeCreationView->SetObject(
		    modelingSpec->GetAttributeConstructionSpec()->GetAttributeCreationParameters());
		attributeCreationView->Open();
	}
}

void KWModelingAdvancedSpecView::InspectAttributePairsParameters()
{
	KWModelingSpec* modelingSpec;
	KWAttributePairsSpecView attributePairsSpecView;

	// Acces a l'objet edite
	modelingSpec = cast(KWModelingSpec*, GetObject());
	check(modelingSpec);

	// Ouverture de la sous-fiche
	attributePairsSpecView.SetObject(modelingSpec->GetAttributeConstructionSpec()->GetAttributePairsSpec());
	attributePairsSpecView.Open();

	// Supression des paires en doublon
	modelingSpec->GetAttributeConstructionSpec()->GetAttributePairsSpec()->DeleteDuplicateAttributePairs();

	// Verification que le nombre de paires max est superieure ou egal au nombre de paires specifiques
	modelingSpec->GetAttributeConstructionSpec()->GetAttributePairsSpec()->CheckAttributePairNumbers();
}

void KWModelingAdvancedSpecView::SetObject(Object* object)
{
	KWModelingSpec* modelingSpec;

	require(object != NULL);

	// Acces a l'objet edite
	modelingSpec = cast(KWModelingSpec*, object);

	// Parametrages des sous-fiches par les sous-objets
	cast(SNBPredictorSelectiveNaiveBayesView*, GetFieldAt("SelectiveNaiveBayesParameters"))
	    ->SetObject(modelingSpec->GetPredictorSelectiveNaiveBayes());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

// ##
