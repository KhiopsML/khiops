// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "MYAnalysisResultsView.h"

MYAnalysisResultsView::MYAnalysisResultsView()
{
	SetIdentifier("MYAnalysisResults");
	SetLabel("Analysis results");
	AddBooleanField("ComputeBasicSecondaryStats", "Compute basic secondary stats", false);

	// ## Custom constructor

	// Deplacement des nouveaux champs vers le haut, avant ceux de la classe ancetre
	ALString sPreparationFieldId = "PreparationFileName";
	MoveFieldBefore("ComputeBasicSecondaryStats", sPreparationFieldId);

	// ##
}

MYAnalysisResultsView::~MYAnalysisResultsView()
{
	// ## Custom destructor

	// ##
}

MYAnalysisResults* MYAnalysisResultsView::GetMYAnalysisResults()
{
	require(objValue != NULL);
	return cast(MYAnalysisResults*, objValue);
}

void MYAnalysisResultsView::EventUpdate(Object* object)
{
	MYAnalysisResults* editedObject;

	require(object != NULL);

	KWAnalysisResultsView::EventUpdate(object);
	editedObject = cast(MYAnalysisResults*, object);
	editedObject->SetComputeBasicSecondaryStats(GetBooleanValueAt("ComputeBasicSecondaryStats"));

	// ## Custom update

	// ##
}

void MYAnalysisResultsView::EventRefresh(Object* object)
{
	MYAnalysisResults* editedObject;

	require(object != NULL);

	KWAnalysisResultsView::EventRefresh(object);
	editedObject = cast(MYAnalysisResults*, object);
	SetBooleanValueAt("ComputeBasicSecondaryStats", editedObject->GetComputeBasicSecondaryStats());

	// ## Custom refresh

	// ##
}

const ALString MYAnalysisResultsView::GetClassLabel() const
{
	return "Analysis results";
}

// ## Method implementation

// ##
