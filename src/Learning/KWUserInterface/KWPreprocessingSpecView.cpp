// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWPreprocessingSpecView.h"

KWPreprocessingSpecView::KWPreprocessingSpecView()
{
	SetIdentifier("KWPreprocessingSpec");
	SetLabel("Preprocessing parameters");
	AddStringField("ObjectLabel", "Preprocessing", "");
	AddBooleanField("TargetGrouped", "Group target values", false);

	// Parametrage des styles;
	GetFieldAt("TargetGrouped")->SetStyle("CheckBox");

	// ## Custom constructor

	GetFieldAt("ObjectLabel")->SetVisible(false);
	AddCardField("DiscretizerSpec", "Discretization", new KWDiscretizerSpecView);
	AddCardField("GrouperSpec", "Value grouping", new KWGrouperSpecView);
	AddCardField("DataGridOptimizerParameters", "Data Grid optimizer parameters",
		     new KWDataGridOptimizerParametersView);

	// Passage en ergonomie onglet
	SetStyle("TabbedPanes");

	// Le parametrage expert n'est visible qu'en mode expert
	GetFieldAt("DataGridOptimizerParameters")->SetVisible(GetLearningExpertMode());

	// Info-bulles
	GetFieldAt("TargetGrouped")
	    ->SetHelpText(
		"Group target values in case of classification task."
		"\n Indicates that the preprocessing methods should consider building discretization by partitioning"
		"\n both the input values (in intervals or groups of values) and the target values into groups."
		"\n This is potentially useful in case of classification tasks with numerous target values,"
		"\n by automatically and optimally reducing the number of target values using groups.");

	// Short cuts
	GetFieldAt("DiscretizerSpec")->SetShortCut('Z');
	GetFieldAt("GrouperSpec")->SetShortCut('G');
	GetFieldAt("DataGridOptimizerParameters")->SetShortCut('M');

	// ##
}

KWPreprocessingSpecView::~KWPreprocessingSpecView()
{
	// ## Custom destructor

	// ##
}

KWPreprocessingSpec* KWPreprocessingSpecView::GetKWPreprocessingSpec()
{
	require(objValue != NULL);
	return cast(KWPreprocessingSpec*, objValue);
}

void KWPreprocessingSpecView::EventUpdate(Object* object)
{
	KWPreprocessingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWPreprocessingSpec*, object);
	editedObject->SetTargetGrouped(GetBooleanValueAt("TargetGrouped"));

	// ## Custom update

	// ##
}

void KWPreprocessingSpecView::EventRefresh(Object* object)
{
	KWPreprocessingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWPreprocessingSpec*, object);
	SetStringValueAt("ObjectLabel", editedObject->GetObjectLabel());
	SetBooleanValueAt("TargetGrouped", editedObject->GetTargetGrouped());

	// ## Custom refresh

	// ##
}

const ALString KWPreprocessingSpecView::GetClassLabel() const
{
	return "Preprocessing parameters";
}

// ## Method implementation

void KWPreprocessingSpecView::SetObject(Object* object)
{
	KWPreprocessingSpec* preprocessingSpec;

	require(object != NULL);

	// Acces a l'objet edite
	preprocessingSpec = cast(KWPreprocessingSpec*, object);

	// Parametrage des sous-fiches
	cast(KWDiscretizerSpecView*, GetFieldAt("DiscretizerSpec"))->SetObject(preprocessingSpec->GetDiscretizerSpec());
	cast(KWGrouperSpecView*, GetFieldAt("GrouperSpec"))->SetObject(preprocessingSpec->GetGrouperSpec());
	cast(KWDataGridOptimizerParametersView*, GetFieldAt("DataGridOptimizerParameters"))
	    ->SetObject(preprocessingSpec->GetDataGridOptimizerParameters());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

// ##
