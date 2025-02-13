// Copyright (c) 2023-2025 Orange. All rights reserved.
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
	AddBooleanField("TargetGrouped", "Group target values", false);
	AddIntField("MaxPartNumber", "Max part number", 0);
	AddIntField("MinPartFrequency", "Min part frequency", 0);

	// Parametrage des styles;
	GetFieldAt("TargetGrouped")->SetStyle("CheckBox");
	GetFieldAt("MaxPartNumber")->SetStyle("Spinner");
	GetFieldAt("MinPartFrequency")->SetStyle("Spinner");

	// ## Custom constructor

	// Initialisation du parametrage des histogrammes
	histogramSpecObject = NULL;

	// Contraintes sur les valeurs
	cast(UIIntElement*, GetFieldAt("MaxPartNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxPartNumber"))->SetMaxValue(1000000);
	cast(UIIntElement*, GetFieldAt("MinPartFrequency"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MinPartFrequency"))->SetMaxValue(1000000);

	// Action d'inspection des parametres avances
	AddAction("InspectAdvancedParameters", "Advanced unsupervised parameters",
		  (ActionMethod)(&KWPreprocessingSpecView::InspectAdvancedParameters));
	AddAction("InspectHistogramParameters", "Expert histogram parameters",
		  (ActionMethod)(&KWPreprocessingSpecView::InspectHistogramParameters));
	AddAction("InspectDataGridOptimizerParameters", "Expert bivariate optimizer parameters",
		  (ActionMethod)(&KWPreprocessingSpecView::InspectDataGridOptimizerParameters));

	// Maquettage des actions sous-forme de boutons
	GetActionAt("InspectAdvancedParameters")->SetStyle("Button");
	GetActionAt("InspectHistogramParameters")->SetStyle("Button");
	GetActionAt("InspectDataGridOptimizerParameters")->SetStyle("Button");

	// Parametrage du mode expert
	GetFieldAt("MinPartFrequency")->SetVisible(GetLearningExpertMode());
	GetActionAt("InspectHistogramParameters")->SetVisible(GetLearningExpertMode());
	GetActionAt("InspectDataGridOptimizerParameters")->SetVisible(GetLearningExpertMode());

	// Info-bulles
	GetFieldAt("TargetGrouped")
	    ->SetHelpText(
		"Group target values in case of classification task."
		"\n"
		"\n Indicates that the preprocessing methods should consider building discretization by partitioning"
		"\n both the input values (in intervals or groups of values) and the target values into groups."
		"\n This is potentially useful in case of classification tasks with numerous target values,"
		"\n by automatically and optimally reducing the number of target values using groups.");
	GetFieldAt("MaxPartNumber")
	    ->SetHelpText("Max number of parts produced by preprocessing methods."
			  "\n"
			  "\n When this interpretability constraint is active, it has priority over the criterion of "
			  "the preprocessing method."
			  "\n Default value 0: automatically set by the method"
			  "\n - optimal number of intervals or groups of values in case of MODL supervised or "
			  "unsupervised methods used by default"
			  "\n - 10 intervals in case of EqualWidth or EqualFrequency unsupervised discretization method"
			  "\n - 10 groups in case of BasicGrouping unsupervised value grouping method");
	GetFieldAt("MinPartFrequency")
	    ->SetHelpText(
		"Min number of instances in interval or group of values."
		"\n When this user constraint is active, it has priority over the criterion of the preprocessig method."
		"\n Default value 0: automatically set by the method.");
	GetActionAt("InspectAdvancedParameters")
	    ->SetHelpText("Advanced parameters for unsupervised preprocessing methods.");
	GetActionAt("InspectHistogramParameters")->SetHelpText("Expert parameters for histogram preprocessing method.");
	GetActionAt("InspectDataGridOptimizerParameters")
	    ->SetHelpText("Expert parameters bivariate preprocessing method.");

	// Short cuts
	GetActionAt("InspectAdvancedParameters")->SetShortCut('V');
	GetActionAt("InspectHistogramParameters")->SetShortCut('X');
	GetActionAt("InspectDataGridOptimizerParameters")->SetShortCut('Z');

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
	editedObject->SetMaxPartNumber(GetIntValueAt("MaxPartNumber"));
	editedObject->SetMinPartFrequency(GetIntValueAt("MinPartFrequency"));

	// ## Custom update

	// ##
}

void KWPreprocessingSpecView::EventRefresh(Object* object)
{
	KWPreprocessingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWPreprocessingSpec*, object);
	SetBooleanValueAt("TargetGrouped", editedObject->GetTargetGrouped());
	SetIntValueAt("MaxPartNumber", editedObject->GetMaxPartNumber());
	SetIntValueAt("MinPartFrequency", editedObject->GetMinPartFrequency());

	// ## Custom refresh

	// ##
}

const ALString KWPreprocessingSpecView::GetClassLabel() const
{
	return "Preprocessing parameters";
}

// ## Method implementation

void KWPreprocessingSpecView::InspectAdvancedParameters()
{
	KWPreprocessingSpec* preprocessingSpec;
	KWPreprocessingAdvancedSpecView preprocessingAdvancedSpecView;

	// Edition de l'objet edite via une vue specialisee
	preprocessingSpec = cast(KWPreprocessingSpec*, GetObject());
	preprocessingAdvancedSpecView.SetObject(preprocessingSpec);
	preprocessingAdvancedSpecView.Open();
}

void KWPreprocessingSpecView::InspectHistogramParameters()
{
	MHHistogramSpecView histogramSpecView;

	require(histogramSpecObject != NULL);

	// Ouverture de la sous-fiche
	histogramSpecView.SetObject(histogramSpecObject);
	histogramSpecView.Open();
}

void KWPreprocessingSpecView::InspectDataGridOptimizerParameters()
{
	KWPreprocessingSpec* preprocessingSpec;
	KWDataGridOptimizerParametersView dataGridOptimizerParametersView;

	// Edition de l'objet edite via une vue specialisee
	preprocessingSpec = cast(KWPreprocessingSpec*, GetObject());
	dataGridOptimizerParametersView.SetObject(preprocessingSpec->GetDataGridOptimizerParameters());
	dataGridOptimizerParametersView.Open();
}

void KWPreprocessingSpecView::SetObject(Object* object)
{
	KWPreprocessingSpec* preprocessingSpec;

	require(object != NULL);

	// Acces a l'objet edite
	preprocessingSpec = cast(KWPreprocessingSpec*, object);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

void KWPreprocessingSpecView::SetHistogramSpecObject(Object* object)
{
	histogramSpecObject = object;
}

Object* KWPreprocessingSpecView::GetHistogramSpecObject()
{
	return histogramSpecObject;
}

// ##
