// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWModelingAdvancedSpecView.h"

KWModelingAdvancedSpecView::KWModelingAdvancedSpecView()
{
	SetIdentifier("KWModelingAdvancedSpec");
	SetLabel("Predictor advanced parameters");

	// Ajout des champs
	AddBooleanField("BaselinePredictor", "Baseline predictor", false);
	AddIntField("UnivariatePredictorNumber", "Number of univariate predictors", 0);

	// Parametrage des styles;
	GetFieldAt("BaselinePredictor")->SetStyle("CheckBox");
	GetFieldAt("UnivariatePredictorNumber")->SetStyle("Spinner");

	// Plage de valeurs pour le nombre de predicteurs univaries
	cast(UIIntElement*, GetFieldAt("UnivariatePredictorNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("UnivariatePredictorNumber"))->SetMaxValue(100);

	// Declaration des actions
	// On utilise exceptionnellement un format html pour le libelle des actions, pour l'avoir centre et sur deux
	// lignes
	AddAction("InspectSelectiveNaiveBayesParameters",
		  "<html> <center> Selective Naive Bayes <br> parameters </center> </html>",
		  (ActionMethod)(&KWModelingAdvancedSpecView::InspectSelectiveNaiveBayesParameters));
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
	GetActionAt("InspectSelectiveNaiveBayesParameters")->SetStyle("Button");
	GetActionAt("InspectConstructionDomain")->SetStyle("Button");
	GetActionAt("InspectTextFeaturesParameters")->SetStyle("Button");
	GetActionAt("InspectAttributePairsParameters")->SetStyle("Button");
	GetActionAt("InspectAttributeCreationParameters")->SetStyle("Button");

	// Parametrage des paramtres expert pour les variables de type texte
	GetActionAt("InspectTextFeaturesParameters")->SetVisible(GetLearningTextVariableMode());

	// Action d'edition des parametre des arbres disponible uniquement en mode avance
	GetActionAt("InspectAttributeCreationParameters")
	    ->SetVisible(KDDataPreparationAttributeCreationTask::GetGlobalCreationTask() != NULL and
			 KDDataPreparationAttributeCreationTaskView::GetGlobalCreationTaskView() != NULL);

	// Info-bulles
	GetFieldAt("BaselinePredictor")
	    ->SetHelpText("Build a base line predictor"
			  "\n The baseline classifier predicts the train majority class."
			  "\n The baseline regressor predicts the train mean of the target variable.");
	GetFieldAt("UnivariatePredictorNumber")
	    ->SetHelpText("Number of univariate predictors to build."
			  "\n The univariate predictors are chosen according to their predictive importance,"
			  "\n which is assessed during the analysis of the train database.");
	GetActionAt("InspectSelectiveNaiveBayesParameters")
	    ->SetHelpText(
		"Advanced parameters for the Selective Naive Bayes predictor."
		"\n These parameters are user constraints that allow to control the variable selection process."
		"\n Their use might decrease the performance, compared to the default mode (without user "
		"constraints).");
	GetActionAt("InspectConstructionDomain")
	    ->SetHelpText(
		"Advanced parameters to select the construction rules used for automatic variable construction.");
	GetActionAt("InspectTextFeaturesParameters")
	    ->SetHelpText("Advanced parameters for the construction of text features.");
	GetActionAt("InspectAttributePairsParameters")
	    ->SetHelpText("Advanced parameters to select the variable pairs to analyze.");
	GetActionAt("InspectAttributeCreationParameters")
	    ->SetHelpText("Advanced parameters for the constuction of tree based variables.");
}

KWModelingAdvancedSpecView::~KWModelingAdvancedSpecView() {}

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
	editedObject->SetBaselinePredictor(GetBooleanValueAt("BaselinePredictor"));
	editedObject->SetUnivariatePredictorNumber(GetIntValueAt("UnivariatePredictorNumber"));
}

void KWModelingAdvancedSpecView::EventRefresh(Object* object)
{
	KWModelingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWModelingSpec*, object);
	SetBooleanValueAt("BaselinePredictor", editedObject->GetBaselinePredictor());
	SetIntValueAt("UnivariatePredictorNumber", editedObject->GetUnivariatePredictorNumber());
}

const ALString KWModelingAdvancedSpecView::GetClassLabel() const
{
	return "Advanced predictor parameters";
}

void KWModelingAdvancedSpecView::InspectSelectiveNaiveBayesParameters()
{
	KWModelingSpec* modelingSpec;
	const SNBPredictorSelectiveNaiveBayesView refPredictorSelectiveNaiveBayesView;
	KWPredictorView* predictorSelectiveNaiveBayesView;

	// Acces a l'objet edite
	modelingSpec = cast(KWModelingSpec*, GetObject());
	check(modelingSpec);

	// Creation de la fiche specialise pour le predicteur
	predictorSelectiveNaiveBayesView =
	    KWPredictorView::ClonePredictorView(refPredictorSelectiveNaiveBayesView.GetName());

	// Ouverture de la sous-fiche
	predictorSelectiveNaiveBayesView->SetObject(modelingSpec->GetPredictorSelectiveNaiveBayes());
	predictorSelectiveNaiveBayesView->Open();

	// Nettoyage
	delete predictorSelectiveNaiveBayesView;
}

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

	// Verification que le nombre de paires max est superieure ou egal au nombre de paires specifique
	modelingSpec->GetAttributeConstructionSpec()->GetAttributePairsSpec()->CheckAttributePairNumbers();
}
