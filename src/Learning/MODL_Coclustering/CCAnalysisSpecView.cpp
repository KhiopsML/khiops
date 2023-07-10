// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCAnalysisSpecView.h"

CCAnalysisSpecView::CCAnalysisSpecView()
{
	SetIdentifier("CCAnalysisSpec");
	SetLabel("Parameters");

	// ## Custom constructor

	// Ajout des sous-fiches
	AddCardField("CoclusteringParameters", "Coclustering parameters", new CCCoclusteringSpecView);
	AddCardField("SystemParameters", "System parameters", new KWSystemParametersView);
	cast(KWSystemParametersView*, GetFieldAt("SystemParameters"))
	    ->GetFieldAt("MaxItemNumberInReports")
	    ->SetVisible(false);
	AddCardField("CrashTestParameters", "Crash test parameters", new KWCrashTestParametersView);

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

	// ##
}

CCAnalysisSpecView::~CCAnalysisSpecView()
{
	// ## Custom destructor

	// ##
}

CCAnalysisSpec* CCAnalysisSpecView::GetCCAnalysisSpec()
{
	require(objValue != NULL);
	return cast(CCAnalysisSpec*, objValue);
}

void CCAnalysisSpecView::EventUpdate(Object* object)
{
	CCAnalysisSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCAnalysisSpec*, object);

	// ## Custom update

	// ##
}

void CCAnalysisSpecView::EventRefresh(Object* object)
{
	CCAnalysisSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCAnalysisSpec*, object);

	// ## Custom refresh

	// ##
}

const ALString CCAnalysisSpecView::GetClassLabel() const
{
	return "Parameters";
}

// ## Method implementation

void CCAnalysisSpecView::SetObject(Object* object)
{
	CCAnalysisSpec* analysisSpec;

	require(object != NULL);

	// Acces a l'objet edite
	analysisSpec = cast(CCAnalysisSpec*, object);

	// Parametrages des sous-fiches par les sous-objets
	cast(CCCoclusteringSpecView*, GetFieldAt("CoclusteringParameters"))
	    ->SetObject(analysisSpec->GetCoclusteringSpec());

	// Cas particulier de la vue sur les parametres systemes
	// Cette vue ne travaille sur des variables statiques de KWSystemResource et KWLearningSpec
	// On lui passe neanmoins en parametre un objet quelconque non NUL pour forcer les mises a jour
	// entre donnees et interface
	cast(KWSystemParametersView*, GetFieldAt("SystemParameters"))->SetObject(object);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

// ##
