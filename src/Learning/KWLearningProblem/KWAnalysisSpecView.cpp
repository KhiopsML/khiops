// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAnalysisSpecView.h"

KWAnalysisSpecView::KWAnalysisSpecView()
{
	SetIdentifier("KWAnalysisSpec");
	SetLabel("Parameters");
	AddStringField("TargetAttributeName", "Target variable", "");
	AddStringField("MainTargetModality", "Main target value", "");

	// ## Custom constructor

	// Ajout de sous-fiches
	AddCardField("PredictorsSpec", "Predictors", new KWModelingSpecView);
	AddCardField("RecodersSpec", "Recoders", new KWRecoderSpecView);
	AddCardField("PreprocessingSpec", "Preprocessing", new KWPreprocessingSpecView);
	AddCardField("SystemParameters", "System parameters", new KWSystemParametersView);
	AddCardField("CrashTestParameters", "Crash test parameters", new KWCrashTestParametersView);

	// Parametrage de la visibilite de l'onglet des crash tests
	GetFieldAt("CrashTestParameters")->SetVisible(GetLearningCrashTestMode());

	// Passage en ergonomie onglets
	SetStyle("TabbedPanes");

	// Info-bulles
	GetFieldAt("TargetAttributeName")
	    ->SetHelpText("Name of the target variable."
			  "\n The learning task is classification if the target variable is categorical,"
			  "\n regression if it is numerical."
			  "\n If the target variable is not specified, the task is unsupervised learning.");
	GetFieldAt("MainTargetModality")
	    ->SetHelpText("Value of the target variable in case of classification,"
			  "\n for the lift curves in the evaluation reports.");

	// Short cuts
	GetFieldAt("PredictorsSpec")->SetShortCut('E');
	GetFieldAt("RecodersSpec")->SetShortCut('O');
	GetFieldAt("PreprocessingSpec")->SetShortCut('G');
	GetFieldAt("SystemParameters")->SetShortCut('S');
	GetFieldAt("CrashTestParameters")->SetShortCut('C');

	// ##
}

KWAnalysisSpecView::~KWAnalysisSpecView()
{
	// ## Custom destructor

	// ##
}

KWAnalysisSpec* KWAnalysisSpecView::GetKWAnalysisSpec()
{
	require(objValue != NULL);
	return cast(KWAnalysisSpec*, objValue);
}

void KWAnalysisSpecView::EventUpdate(Object* object)
{
	KWAnalysisSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAnalysisSpec*, object);
	editedObject->SetTargetAttributeName(GetStringValueAt("TargetAttributeName"));
	editedObject->SetMainTargetModality(GetStringValueAt("MainTargetModality"));

	// ## Custom update

	// ##
}

void KWAnalysisSpecView::EventRefresh(Object* object)
{
	KWAnalysisSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAnalysisSpec*, object);
	SetStringValueAt("TargetAttributeName", editedObject->GetTargetAttributeName());
	SetStringValueAt("MainTargetModality", editedObject->GetMainTargetModality());

	// ## Custom refresh

	// ##
}

const ALString KWAnalysisSpecView::GetClassLabel() const
{
	return "Parameters";
}

// ## Method implementation

const ALString KWAnalysisSpecView::GetObjectLabel() const
{
	return "";
}

void KWAnalysisSpecView::SetObject(Object* object)
{
	KWAnalysisSpec* analysisSpec;

	require(object != NULL);

	// Acces a l'objet edite
	analysisSpec = cast(KWAnalysisSpec*, object);

	// Parametrages des sous-fiches par les sous-objets
	cast(KWModelingSpecView*, GetFieldAt("PredictorsSpec"))->SetObject(analysisSpec->GetModelingSpec());
	cast(KWRecoderSpecView*, GetFieldAt("RecodersSpec"))->SetObject(analysisSpec->GetRecoderSpec());
	cast(KWPreprocessingSpecView*, GetFieldAt("PreprocessingSpec"))
	    ->SetObject(analysisSpec->GetPreprocessingSpec());

	// Parametrage specifique de l'objet de parametrage des histogrammes, qui poiur des raison techniques (libriaire
	// independante) est contenu dans les analysisSpec et non dans les preprocessingSpec
	cast(KWPreprocessingSpecView*, GetFieldAt("PreprocessingSpec"))
	    ->SetHistogramSpecObject(analysisSpec->GetHistogramSpec());

	// Cas particulier de la vue sur les parametres systemes
	// Cette vue ne travaille sur des variables statiques de KWSystemResource et KWLearningSpec
	// On lui passe neanmoins en parametre un objet quelconque non NUL pour forcer les mises a jour
	// entre donnees et interface
	cast(KWSystemParametersView*, GetFieldAt("SystemParameters"))->SetObject(object);

	// Idem pour les parametre de crash test
	cast(KWCrashTestParametersView*, GetFieldAt("CrashTestParameters"))->SetObject(object);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

// ##
