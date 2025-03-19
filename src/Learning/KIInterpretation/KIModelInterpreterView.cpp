// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIModelInterpreterView.h"

KIModelInterpreterView::KIModelInterpreterView()
{
	SetIdentifier("KIModelInterpreter");
	SetLabel("Interpret model");
	AddStringField("ShapleyValueRanking", "Shapley values ranking", "");
	AddIntField("ContributionAttributeNumber", "Number of contribution variables", 0);

	// Parametrage des styles;
	GetFieldAt("ShapleyValueRanking")->SetStyle("ComboBox");
	GetFieldAt("ContributionAttributeNumber")->SetStyle("Spinner");

	// ## Custom constructor

	// Types de ranking pour les valeurs de Shapley
	GetFieldAt("ShapleyValueRanking")->SetParameters("Global\nIndividual");

	// Parametrage des nombres min et max
	cast(UIIntElement*, GetFieldAt("ContributionAttributeNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("ContributionAttributeNumber"))
	    ->SetMaxValue(KIModelInterpreter::nMaxContributionAttributeNumber);

	// Ajout de l'action de construction d'un dictionnaire d'interpretation
	AddAction("BuildInterpretationClass", "Build interpretation dictionary...",
		  (ActionMethod)(&KIModelInterpreterView::BuildInterpretationClass));
	GetActionAt("BuildInterpretationClass")->SetStyle("Button");

	// Info-bulles
	GetFieldAt("ShapleyValueRanking")
	    ->SetHelpText(
		"Ranking of the Shapley values produced by the interpretation model"
		"\n- Global: predictor variables are ranked by decreasing global importance"
		"\n and one Shapley value is written per target value and predictor variable, by decreasing variable "
		"rank"
		"\n- Individual: predictor variables are ranked by decreasing individual Shapley values"
		"\n and three importance variables are written by target values by decreasing Shapley value:"
		"\n name of predictor variable, variable part and Shapley value");
	GetFieldAt("ContributionAttributeNumber")
	    ->SetHelpText("Number of predictor variables exploited the interpretation model");
	GetActionAt("BuildInterpretationClass")
	    ->SetHelpText("Build an interpretation dictionary that computes the Shapley values");

	// ##
}

KIModelInterpreterView::~KIModelInterpreterView()
{
	// ## Custom destructor

	// ##
}

KIModelInterpreter* KIModelInterpreterView::GetKIModelInterpreter()
{
	require(objValue != NULL);
	return cast(KIModelInterpreter*, objValue);
}

void KIModelInterpreterView::EventUpdate(Object* object)
{
	KIModelInterpreter* editedObject;

	require(object != NULL);

	KIModelServiceView::EventUpdate(object);
	editedObject = cast(KIModelInterpreter*, object);
	editedObject->SetShapleyValueRanking(GetStringValueAt("ShapleyValueRanking"));
	editedObject->SetContributionAttributeNumber(GetIntValueAt("ContributionAttributeNumber"));

	// ## Custom update

	// ##
}

void KIModelInterpreterView::EventRefresh(Object* object)
{
	KIModelInterpreter* editedObject;

	require(object != NULL);

	KIModelServiceView::EventRefresh(object);
	editedObject = cast(KIModelInterpreter*, object);
	SetStringValueAt("ShapleyValueRanking", editedObject->GetShapleyValueRanking());
	SetIntValueAt("ContributionAttributeNumber", editedObject->GetContributionAttributeNumber());

	// ## Custom refresh

	// ##
}

const ALString KIModelInterpreterView::GetClassLabel() const
{
	return "Interpret model";
}

// ## Method implementation

void KIModelInterpreterView::BuildInterpretationClass()
{
	AddSimpleMessage("Not yet implemented");
}

// ##
