// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWRecodingSpecView.h"

KWRecodingSpecView::KWRecodingSpecView()
{
	SetIdentifier("KWRecodingSpec");
	SetLabel("Recoding parameters");
	AddBooleanField("FilterAttributes", "Keep informative variables only", false);
	AddIntField("MaxFilteredAttributeNumber", "Max number of filtered variables", 0);
	AddBooleanField("KeepInitialSymbolAttributes", "Keep initial categorical variables", false);
	AddBooleanField("KeepInitialContinuousAttributes", "Keep initial numerical variables", false);
	AddStringField("RecodeSymbolAttributes", "Categorical recoding method", "");
	AddStringField("RecodeContinuousAttributes", "Numerical recoding method", "");
	AddStringField("RecodeBivariateAttributes", "Pairs recoding method", "");
	AddBooleanField("RecodeProbabilisticDistance", "Recode using prob distance (expert)", false);

	// Parametrage des styles;
	GetFieldAt("FilterAttributes")->SetStyle("CheckBox");
	GetFieldAt("MaxFilteredAttributeNumber")->SetStyle("Spinner");
	GetFieldAt("KeepInitialSymbolAttributes")->SetStyle("CheckBox");
	GetFieldAt("KeepInitialContinuousAttributes")->SetStyle("CheckBox");
	GetFieldAt("RecodeSymbolAttributes")->SetStyle("ComboBox");
	GetFieldAt("RecodeContinuousAttributes")->SetStyle("ComboBox");
	GetFieldAt("RecodeBivariateAttributes")->SetStyle("ComboBox");
	GetFieldAt("RecodeProbabilisticDistance")->SetStyle("CheckBox");

	// ## Custom constructor

	// L'etude des distance probabilistes n'est disponible qu'en mode expert
	GetFieldAt("RecodeProbabilisticDistance")->SetVisible(GetDistanceStudyMode());

	// Parametrage des champs
	cast(UIIntElement*, GetFieldAt("MaxFilteredAttributeNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxFilteredAttributeNumber"))->SetMaxValue(1000000);
	GetFieldAt("RecodeSymbolAttributes")
	    ->SetParameters("part Id\npart label\n0-1 binarization\nconditional info\nnone");
	GetFieldAt("RecodeContinuousAttributes")
	    ->SetParameters("part Id\npart label\n0-1 binarization\nconditional info\ncenter-reduction\n0-1 "
			    "normalization\nrank normalization\nnone");
	GetFieldAt("RecodeBivariateAttributes")
	    ->SetParameters("part Id\npart label\n0-1 binarization\nconditional info\nnone");

	// Info-bulles
	GetFieldAt("FilterAttributes")
	    ->SetHelpText("If true, all the non informative variables are discarded,"
			  "\n in their initial or recoded representation");
	GetFieldAt("MaxFilteredAttributeNumber")
	    ->SetHelpText(
		"Max number of variables originating from the data preparation, to keep in the recoder."
		"\n The filtered variables are those having the highest univariate predictive importance (Level)."
		"\n (default: 0, means that all the variables are kept).");
	GetFieldAt("KeepInitialSymbolAttributes")
	    ->SetHelpText("Keep the initial categorical variables before preprocessing.");
	GetFieldAt("KeepInitialContinuousAttributes")
	    ->SetHelpText("Keep the initial numerical variables before preprocessing.");
	GetFieldAt("RecodeSymbolAttributes")
	    ->SetHelpText("Categorical recoding method."
			  "\n - part Id: identifier of the part (group of values)"
			  "\n - part label: comprehensible label of the part, like in reports"
			  "\n - binarization: 0-1 binarization of the part (generates as many Boolean variables as "
			  "number of groups of values)"
			  "\n - conditional info: negative log of the conditional probability of the source variable "
			  "given the target variable (-log(p(X|Y))"
			  "\n - none: do not recode the variable");
	GetFieldAt("RecodeContinuousAttributes")
	    ->SetHelpText(
		"Numerical recoding method."
		"\n - part Id: identifier of the part (interval)"
		"\n - part label: comprehensible label of the part, like in reports"
		"\n - binarization: 0-1 binarization of the part (generates as many Boolean variables as number of "
		"intervals)"
		"\n - conditional info: negative log of the conditional probability of the source variable given the "
		"target variable (-log(p(X|Y))."
		"\n - center-reduction: (X - Mean(X)) / StdDev(X)"
		"\n - normalization: (X - Min(X)) / (Max(X) - Min(X))"
		"\n - rank normalization: mean normalized rank (rank between 0 and 1) of the instances of the interval"
		"\n - none: do not recode the variable");
	GetFieldAt("RecodeBivariateAttributes")
	    ->SetHelpText("Pairs recoding method."
			  "\n - part Id: identifier of the part (bivariate cell)"
			  "\n - part label: comprehensible label of the part, like in reports"
			  "\n - binarization: 0-1 binarization of the part (generates as many Boolean variables as "
			  "number of cells)"
			  "\n - conditional info: negative log of the conditional probability of the source variable "
			  "given the target variable (-log(p(X|Y))"
			  "\n - none: do not recode the variable pair");

	// ##
}

KWRecodingSpecView::~KWRecodingSpecView()
{
	// ## Custom destructor

	// ##
}

KWRecodingSpec* KWRecodingSpecView::GetKWRecodingSpec()
{
	require(objValue != NULL);
	return cast(KWRecodingSpec*, objValue);
}

void KWRecodingSpecView::EventUpdate(Object* object)
{
	KWRecodingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWRecodingSpec*, object);
	editedObject->SetFilterAttributes(GetBooleanValueAt("FilterAttributes"));
	editedObject->SetMaxFilteredAttributeNumber(GetIntValueAt("MaxFilteredAttributeNumber"));
	editedObject->SetKeepInitialSymbolAttributes(GetBooleanValueAt("KeepInitialSymbolAttributes"));
	editedObject->SetKeepInitialContinuousAttributes(GetBooleanValueAt("KeepInitialContinuousAttributes"));
	editedObject->SetRecodeSymbolAttributes(GetStringValueAt("RecodeSymbolAttributes"));
	editedObject->SetRecodeContinuousAttributes(GetStringValueAt("RecodeContinuousAttributes"));
	editedObject->SetRecodeBivariateAttributes(GetStringValueAt("RecodeBivariateAttributes"));
	editedObject->SetRecodeProbabilisticDistance(GetBooleanValueAt("RecodeProbabilisticDistance"));

	// ## Custom update

	// ##
}

void KWRecodingSpecView::EventRefresh(Object* object)
{
	KWRecodingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWRecodingSpec*, object);
	SetBooleanValueAt("FilterAttributes", editedObject->GetFilterAttributes());
	SetIntValueAt("MaxFilteredAttributeNumber", editedObject->GetMaxFilteredAttributeNumber());
	SetBooleanValueAt("KeepInitialSymbolAttributes", editedObject->GetKeepInitialSymbolAttributes());
	SetBooleanValueAt("KeepInitialContinuousAttributes", editedObject->GetKeepInitialContinuousAttributes());
	SetStringValueAt("RecodeSymbolAttributes", editedObject->GetRecodeSymbolAttributes());
	SetStringValueAt("RecodeContinuousAttributes", editedObject->GetRecodeContinuousAttributes());
	SetStringValueAt("RecodeBivariateAttributes", editedObject->GetRecodeBivariateAttributes());
	SetBooleanValueAt("RecodeProbabilisticDistance", editedObject->GetRecodeProbabilisticDistance());

	// ## Custom refresh

	// ##
}

const ALString KWRecodingSpecView::GetClassLabel() const
{
	return "Recoding parameters";
}

// ## Method implementation

// ##
