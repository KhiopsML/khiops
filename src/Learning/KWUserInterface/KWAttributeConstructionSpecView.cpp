// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAttributeConstructionSpecView.h"

KWAttributeConstructionSpecView::KWAttributeConstructionSpecView()
{
	SetIdentifier("KWAttributeConstructionSpec");
	SetLabel("Feature engineering parameters");
	AddBooleanField("KeepSelectedAttributesOnly", "Keep selected variables only", false);
	AddIntField("MaxConstructedAttributeNumber", "Max number of constructed variables", 0);
	AddIntField("MaxTextFeatureNumber", "Max number of text features", 0);
	AddIntField("MaxTreeNumber", "Max number of trees", 0);
	AddIntField("MaxAttributePairNumber", "Max number of variable pairs", 0);

	// Parametrage des styles;
	GetFieldAt("KeepSelectedAttributesOnly")->SetStyle("CheckBox");
	GetFieldAt("MaxConstructedAttributeNumber")->SetStyle("Spinner");
	GetFieldAt("MaxTextFeatureNumber")->SetStyle("Spinner");
	GetFieldAt("MaxTreeNumber")->SetStyle("Spinner");
	GetFieldAt("MaxAttributePairNumber")->SetStyle("Spinner");

	// ## Custom constructor

	// Parametrage des nombre min et max
	cast(UIIntElement*, GetFieldAt("MaxConstructedAttributeNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxConstructedAttributeNumber"))
	    ->SetMaxValue(KWAttributeConstructionSpec::nLargestMaxConstructedAttributeNumber);
	cast(UIIntElement*, GetFieldAt("MaxTextFeatureNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxTextFeatureNumber"))
	    ->SetMaxValue(KWAttributeConstructionSpec::nLargestMaxTextFeatureNumber);
	cast(UIIntElement*, GetFieldAt("MaxTreeNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxTreeNumber"))
	    ->SetMaxValue(KWAttributeConstructionSpec::nLargestMaxTreeNumber);
	cast(UIIntElement*, GetFieldAt("MaxAttributePairNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxAttributePairNumber"))
	    ->SetMaxValue(KWAttributePairsSpec::nLargestMaxAttributePairNumber);

	// Parametrage de la construction des arbres
	GetFieldAt("MaxTreeNumber")
	    ->SetVisible(KDDataPreparationAttributeCreationTask::GetGlobalCreationTask() != NULL);

	// Info-bulles
	GetFieldAt("KeepSelectedAttributesOnly")
	    ->SetHelpText("In supervised analysis, constructed variables are only retained in the "
			  "\n data preparation reports if they are selected in the Naive Bayes Selective predictor.");
	GetFieldAt("MaxConstructedAttributeNumber")
	    ->SetHelpText("Max number of variables to construct."
			  "\n The constructed variables allow to extract numerical or categorical values"
			  "\n resulting from computing formula applied to existing variables in secondary tables"
			  "\n (e.g. YearDay of a Date variable, Mean of a Numerical variable from a Table Variable).");
	GetFieldAt("MaxTextFeatureNumber")
	    ->SetHelpText("Max number of text features to construct."
			  "\n Text features are constructed from variables of type Text or TextList available in "
			  "either the main table or"
			  "\n a secondary table. The features are constructed by default using ngrams of bytes.");
	GetFieldAt("MaxTreeNumber")
	    ->SetHelpText("Max number of trees to construct."
			  "\n The constructed trees allow to combine variables, either native or constructed."
			  "\n Construction of trees is not available in regression analysis.");
	GetFieldAt("MaxAttributePairNumber")
	    ->SetHelpText("Max number of variable pairs to analyze during data preparation."
			  "\n The variable pairs are preprocessed using a bivariate discretization method."
			  "\n Pairs of variable are not available in regression analysis");

	// ##
}

KWAttributeConstructionSpecView::~KWAttributeConstructionSpecView()
{
	// ## Custom destructor

	// ##
}

KWAttributeConstructionSpec* KWAttributeConstructionSpecView::GetKWAttributeConstructionSpec()
{
	require(objValue != NULL);
	return cast(KWAttributeConstructionSpec*, objValue);
}

void KWAttributeConstructionSpecView::EventUpdate(Object* object)
{
	KWAttributeConstructionSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeConstructionSpec*, object);
	editedObject->SetKeepSelectedAttributesOnly(GetBooleanValueAt("KeepSelectedAttributesOnly"));
	editedObject->SetMaxConstructedAttributeNumber(GetIntValueAt("MaxConstructedAttributeNumber"));
	editedObject->SetMaxTextFeatureNumber(GetIntValueAt("MaxTextFeatureNumber"));
	editedObject->SetMaxTreeNumber(GetIntValueAt("MaxTreeNumber"));
	editedObject->SetMaxAttributePairNumber(GetIntValueAt("MaxAttributePairNumber"));

	// ## Custom update

	// ##
}

void KWAttributeConstructionSpecView::EventRefresh(Object* object)
{
	KWAttributeConstructionSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeConstructionSpec*, object);
	SetBooleanValueAt("KeepSelectedAttributesOnly", editedObject->GetKeepSelectedAttributesOnly());
	SetIntValueAt("MaxConstructedAttributeNumber", editedObject->GetMaxConstructedAttributeNumber());
	SetIntValueAt("MaxTextFeatureNumber", editedObject->GetMaxTextFeatureNumber());
	SetIntValueAt("MaxTreeNumber", editedObject->GetMaxTreeNumber());
	SetIntValueAt("MaxAttributePairNumber", editedObject->GetMaxAttributePairNumber());

	// ## Custom refresh

	// ##
}

const ALString KWAttributeConstructionSpecView::GetClassLabel() const
{
	return "Feature engineering parameters";
}

// ## Method implementation

// ##