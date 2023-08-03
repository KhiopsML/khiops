// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCAnalysisSpecView.h"

CCAnalysisSpecView::CCAnalysisSpecView()
{
	SetIdentifier("CCAnalysisSpec");
	SetLabel("Parameters");

	// CH IV Begin
	// CH IV Refactoring: plutot passer par une gestion standard des parametres
	//    dans le constructeur, EventRefresh, EventUpdate SetObject
	//    et juste conditionner le visibilites des champs concernes
	// Ajout d'un radio bouton pour choisir le type de coclustering : variables*variables ou instances*variables
	// Le choix est exclusif
	AddStringField("CoclusteringType", "Coclustering Type", "");
	GetFieldAt("CoclusteringType")->SetStyle("RadioButton");
	GetFieldAt("CoclusteringType")
	    ->SetParameters(CCAnalysisSpec::GetCoclusteringLabelFromType(false) + "\n" +
			    CCAnalysisSpec::GetCoclusteringLabelFromType(true));
	cast(UIStringElement*, GetFieldAt("CoclusteringType"))
	    ->SetDefaultValue(CCAnalysisSpec::GetCoclusteringLabelFromType(false));
	// CH IV End

	// Ajout des sous-fiches
	AddCardField("CoclusteringParameters", "Variables parameters", new CCCoclusteringSpecView);
	// CH IV Begin
	AddCardField("VarPartCoclusteringParameters", "Instances x variables parameters",
		     new CCVarPartCoclusteringSpecView);
	// CH IV End
	AddCardField("SystemParameters", "System parameters", new KWSystemParametersView);
	AddCardField("CrashTestParameters", "Crash test parameters", new KWCrashTestParametersView);

	// CH IV Begin
	// Parametrage de la visibilite du coclustering instances*variables
	GetFieldAt("CoclusteringType")->SetVisible(GetLearningCoclusteringIVExpertMode());
	GetFieldAt("VarPartCoclusteringParameters")->SetVisible(GetLearningCoclusteringIVExpertMode());
	// CH IV End

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

	// CH IV Begin
	editedObject->SetVarPartCoclustering(
	    editedObject->GetCoclusteringTypeFromLabel(GetStringValueAt("CoclusteringType")));
	// CH IV End
}

void CCAnalysisSpecView::EventRefresh(Object* object)
{
	CCAnalysisSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCAnalysisSpec*, object);

	// CH IV Begin
	SetStringValueAt("CoclusteringType",
			 editedObject->GetCoclusteringLabelFromType(editedObject->GetVarPartCoclustering()));
	// CH IV End
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

	// CH IV Begin
	cast(CCVarPartCoclusteringSpecView*, GetFieldAt("VarPartCoclusteringParameters"))
	    ->SetObject(analysisSpec->GetVarPartCoclusteringSpec());
	// CH IV End

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
