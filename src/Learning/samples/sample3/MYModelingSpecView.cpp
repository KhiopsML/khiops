// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "MYModelingSpecView.h"

MYModelingSpecView::MYModelingSpecView()
{
	SetIdentifier("MYModelingSpec");
	SetLabel("Modeling parameters");
	AddBooleanField("SelectiveNaiveBayesPredictor", "Selective Naive Bayes predictor", false);
	AddBooleanField("NaiveBayesPredictor", "Naive Bayes predictor", false);

	// Parametrage des styles;
	GetFieldAt("SelectiveNaiveBayesPredictor")->SetStyle("CheckBox");
	GetFieldAt("NaiveBayesPredictor")->SetStyle("CheckBox");

	// ## Custom constructor

	// Deplacement des nouveaux champs vers le haut, avant ceux de la classe ancetre
	ALString sFirstFieldId = GetFieldAtIndex(0)->GetIdentifier();
	MoveFieldBefore("SelectiveNaiveBayesPredictor", sFirstFieldId);
	MoveFieldBefore("NaiveBayesPredictor", sFirstFieldId);

	// Declaration des actions
	AddAction("InspectSelectiveNaiveBayesParameters", "Selective Naive Bayes expert parameters",
		  (ActionMethod)(&MYModelingSpecView::InspectSelectiveNaiveBayesParameters));
	GetActionAt("InspectSelectiveNaiveBayesParameters")->SetStyle("Button");

	// ##
}

MYModelingSpecView::~MYModelingSpecView()
{
	// ## Custom destructor

	// ##
}

MYModelingSpec* MYModelingSpecView::GetMYModelingSpec()
{
	require(objValue != NULL);
	return cast(MYModelingSpec*, objValue);
}

void MYModelingSpecView::EventUpdate(Object* object)
{
	MYModelingSpec* editedObject;

	require(object != NULL);

	KWModelingSpecView::EventUpdate(object);
	editedObject = cast(MYModelingSpec*, object);
	editedObject->SetSelectiveNaiveBayesPredictor(GetBooleanValueAt("SelectiveNaiveBayesPredictor"));
	editedObject->SetNaiveBayesPredictor(GetBooleanValueAt("NaiveBayesPredictor"));

	// ## Custom update

	// ##
}

void MYModelingSpecView::EventRefresh(Object* object)
{
	MYModelingSpec* editedObject;

	require(object != NULL);

	KWModelingSpecView::EventRefresh(object);
	editedObject = cast(MYModelingSpec*, object);
	SetBooleanValueAt("SelectiveNaiveBayesPredictor", editedObject->GetSelectiveNaiveBayesPredictor());
	SetBooleanValueAt("NaiveBayesPredictor", editedObject->GetNaiveBayesPredictor());

	// ## Custom refresh

	// ##
}

const ALString MYModelingSpecView::GetClassLabel() const
{
	return "Modeling parameters";
}

// ## Method implementation

void MYModelingSpecView::InspectSelectiveNaiveBayesParameters()
{
	KWPredictorSelectiveNaiveBayesView predictorSelectiveNaiveBayesView;
	KWModelingSpec* modelingSpec;

	// Acces a l'objet edite
	modelingSpec = cast(KWModelingSpec*, GetObject());
	check(modelingSpec);

	// Ouverture de la sous-fiche
	predictorSelectiveNaiveBayesView.SetObject(modelingSpec->GetPredictorSelectiveNaiveBayes());
	predictorSelectiveNaiveBayesView.Open();
}

// ##
