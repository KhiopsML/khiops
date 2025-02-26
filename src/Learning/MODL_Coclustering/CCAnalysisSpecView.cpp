// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCAnalysisSpecView.h"

CCAnalysisSpecView::CCAnalysisSpecView()
{
	SetIdentifier("CCAnalysisSpec");
	SetLabel("Parameters");

	// Ajout d'un radio bouton pour choisir le type de coclustering : variables ou instances*variables
	// Le choix est exclusif
	AddStringField("CoclusteringType", "Coclustering type", "");
	GetFieldAt("CoclusteringType")->SetStyle("RadioButton");
	GetFieldAt("CoclusteringType")
	    ->SetParameters(CCAnalysisSpec::GetCoclusteringLabelFromType(false) + "\n" +
			    CCAnalysisSpec::GetCoclusteringLabelFromType(true));
	cast(UIStringElement*, GetFieldAt("CoclusteringType"))
	    ->SetDefaultValue(CCAnalysisSpec::GetCoclusteringLabelFromType(false));

	// Ajout des sous-fiches
	AddCardField("CoclusteringParameters", "Variables parameters", new CCCoclusteringSpecView);
	AddCardField("DataGridOptimizerParameters", "Data grid parameters", new KWDataGridOptimizerParametersView);
	AddCardField("SystemParameters", "System parameters", new KWSystemParametersView);
	AddCardField("CrashTestParameters", "Crash test parameters", new KWCrashTestParametersView);

	// Parametrage de la visibilite du coclustering instances*variables
	GetFieldAt("CoclusteringType")->SetVisible(GetLearningCoclusteringIVExpertMode());

	// Parametrage de la visibilite de l'onglet des parametres d'optimisation
	GetFieldAt("DataGridOptimizerParameters")->SetVisible(GetLearningExpertMode());

	// Parametrage de la visibilite de l'onglet des crash tests
	GetFieldAt("CrashTestParameters")->SetVisible(GetLearningCrashTestMode());

	// Passage en ergonomie onglets
	SetStyle("TabbedPanes");

	// Parametrage visible du temps minimum d'optimisation pour le coclustering
	cast(KWSystemParametersView*, GetFieldAt("SystemParameters"))->GetFieldAt("OptimizationTime")->SetVisible(true);

	// Parametrage visible du temps minimum d'optimisation pour le coclustreing
	cast(KWSystemParametersView*, GetFieldAt("SystemParameters"))
	    ->GetFieldAt("MaxCoreNumber")
	    ->SetHelpText(
		"Max number of processor cores to use."
		"\n Not used in this version: coclustering algorithms will be parallelized in future versions.");

	// Parametrage avance visible uniquement en mode expert
	cast(KWSystemParametersView*, GetFieldAt("SystemParameters"))
	    ->GetFieldAt("IgnoreMemoryLimit")
	    ->SetVisible(GetLearningExpertMode());

	// Info-bulles
	GetFieldAt("CoclusteringType")
	    ->SetHelpText(
		"Type of coclustering:"
		"\n - Variables coclustering: based on the coclustering variables parameters,"
		"\n - Instances * Variables coclustering: based on an identifier on one dimension, and all "
		"the numerical and categorical variables on the other dimension."
		"\n If a identifier variable of the records is present in the data, it must be a key variable of the "
		"input dictionary and the data must be sorted and unique according to this key."
		"\n If no identifier of the records, such a variable is automatically created."
		"\n For the 'variables' dimension of the coclustering, all numerical and categorical variables"
		"\n used in the input dictionary are employed.");

	// ##

	// Short cuts
	GetFieldAt("CoclusteringParameters")->SetShortCut('C');
	GetFieldAt("SystemParameters")->SetShortCut('S');
}

CCAnalysisSpecView::~CCAnalysisSpecView() {}

void CCAnalysisSpecView::EventUpdate(Object* object)
{
	CCAnalysisSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCAnalysisSpec*, object);
	editedObject->SetVarPartCoclustering(
	    editedObject->GetCoclusteringTypeFromLabel(GetStringValueAt("CoclusteringType")));
}

void CCAnalysisSpecView::EventRefresh(Object* object)
{
	CCAnalysisSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCAnalysisSpec*, object);
	SetStringValueAt("CoclusteringType",
			 editedObject->GetCoclusteringLabelFromType(editedObject->GetVarPartCoclustering()));
}

void CCAnalysisSpecView::SetObject(Object* object)
{
	CCAnalysisSpec* analysisSpec;

	require(object != NULL);

	// Acces a l'objet edite
	analysisSpec = cast(CCAnalysisSpec*, object);

	// Parametrages des sous-fiches par les sous-objets
	cast(CCCoclusteringSpecView*, GetFieldAt("CoclusteringParameters"))
	    ->SetObject(analysisSpec->GetCoclusteringSpec());
	cast(KWDataGridOptimizerParametersView*, GetFieldAt("DataGridOptimizerParameters"))
	    ->SetObject(analysisSpec->GetOptimizationParameters());

	// Cas particulier de la vue sur les parametres systemes
	// Cette vue ne travaille sur des variables statiques de KWSystemResource et KWLearningSpec
	// On lui passe neanmoins en parametre un objet quelconque non NUL pour forcer les mises a jour
	// entre donnees et interface
	cast(KWSystemParametersView*, GetFieldAt("SystemParameters"))->SetObject(object);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

const ALString CCAnalysisSpecView::GetClassLabel() const
{
	return "Parameters";
}
