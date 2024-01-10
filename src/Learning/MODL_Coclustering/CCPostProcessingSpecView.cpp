// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCPostProcessingSpecView.h"

CCPostProcessingSpecView::CCPostProcessingSpecView()
{
	SetIdentifier("CCPostProcessingSpec");
	SetLabel("Simplification parameters");
	AddStringField("ShortDescription", "ShortDescription", "");
	AddIntField("InstanceNumber", "Instance number", 0);
	AddIntField("NonEmptyCellNumber", "Non empty cell number", 0);
	AddIntField("CellNumber", "Cell number", 0);
	AddIntField("MaxCellNumber", "Max cell number", 0);
	AddIntField("MaxPreservedInformation", "Max preserved information", 0);
	AddIntField("TotalPartNumber", "Total part number", 0);
	AddIntField("MaxTotalPartNumber", "Max total part number", 0);
	AddStringField("FrequencyAttribute", "Frequency variable", "");

	// Parametrage des styles;
	GetFieldAt("MaxCellNumber")->SetStyle("Spinner");
	GetFieldAt("MaxPreservedInformation")->SetStyle("Spinner");
	GetFieldAt("MaxTotalPartNumber")->SetStyle("Spinner");

	// ## Custom constructor

	// Les champs caracterisant le coclustering a pretraiter sont non editables
	GetFieldAt("ShortDescription")->SetEditable(false);
	GetFieldAt("InstanceNumber")->SetEditable(false);
	GetFieldAt("NonEmptyCellNumber")->SetEditable(false);
	GetFieldAt("CellNumber")->SetEditable(false);
	GetFieldAt("TotalPartNumber")->SetEditable(false);

	// Ajout d'un tableau des variables de coclustering
	CCPostProcessedAttributeArrayView* postProcessedAttributeArrayView;
	postProcessedAttributeArrayView = new CCPostProcessedAttributeArrayView;
	AddListField("PostProcessedAttributes", "Coclustering variables", postProcessedAttributeArrayView);

	// Seuls les parametres sont editables
	postProcessedAttributeArrayView->SetEditable(false);
	postProcessedAttributeArrayView->GetFieldAt("MaxPartNumber")->SetEditable(true);

	// Plage de valeurs des variables numeriques
	cast(UIIntElement*, GetFieldAt("MaxCellNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxPreservedInformation"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxPreservedInformation"))->SetMaxValue(100);
	cast(UIIntElement*, GetFieldAt("MaxTotalPartNumber"))->SetMinValue(0);
	cast(UIIntElement*, postProcessedAttributeArrayView->GetFieldAt("MaxPartNumber"))->SetMinValue(0);
	cast(UIIntElement*, postProcessedAttributeArrayView->GetFieldAt("MaxPartNumber"))->SetMaxValue(1000000);

	// Le tableau des variable est affiche avant la variable d'effectif
	MoveFieldBefore("PostProcessedAttributes", "FrequencyAttribute");

	// La variable d'effectif n'est pas editable
	GetFieldAt("FrequencyAttribute")->SetEditable(false);

	// Info-bulles
	GetFieldAt("InstanceNumber")->SetHelpText("Instance number in the input coclustering.");
	GetFieldAt("NonEmptyCellNumber")->SetHelpText("Non empty cell number in the input coclustering.");
	GetFieldAt("CellNumber")->SetHelpText("Cell number in the input coclustering.");
	GetFieldAt("MaxCellNumber")
	    ->SetHelpText("Max number of cells to keep"
			  "\n in the simplified coclustering (0: no constraint).");
	GetFieldAt("MaxPreservedInformation")
	    ->SetHelpText("Max percentage of information to keep"
			  "\n in the simplified coclustering (0: no constraint)."
			  "\n Low percentages correspond to weakly informative coarse models whereas"
			  "\n high percentages correspond to highly informative detailed models.");
	GetFieldAt("TotalPartNumber")->SetHelpText("Total part number in the input coclustering.");
	GetFieldAt("MaxTotalPartNumber")
	    ->SetHelpText("Max total number of parts to keep"
			  "\n in the simplified coclustering (0: no constraint).");
	GetFieldAt("FrequencyAttribute")->SetHelpText("Frequency variable in the input coclustering.");

	// ##
}

CCPostProcessingSpecView::~CCPostProcessingSpecView()
{
	// ## Custom destructor

	// ##
}

CCPostProcessingSpec* CCPostProcessingSpecView::GetCCPostProcessingSpec()
{
	require(objValue != NULL);
	return cast(CCPostProcessingSpec*, objValue);
}

void CCPostProcessingSpecView::EventUpdate(Object* object)
{
	CCPostProcessingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCPostProcessingSpec*, object);
	editedObject->SetShortDescription(GetStringValueAt("ShortDescription"));
	editedObject->SetInstanceNumber(GetIntValueAt("InstanceNumber"));
	editedObject->SetNonEmptyCellNumber(GetIntValueAt("NonEmptyCellNumber"));
	editedObject->SetCellNumber(GetIntValueAt("CellNumber"));
	editedObject->SetMaxCellNumber(GetIntValueAt("MaxCellNumber"));
	editedObject->SetMaxPreservedInformation(GetIntValueAt("MaxPreservedInformation"));
	editedObject->SetTotalPartNumber(GetIntValueAt("TotalPartNumber"));
	editedObject->SetMaxTotalPartNumber(GetIntValueAt("MaxTotalPartNumber"));
	editedObject->SetFrequencyAttribute(GetStringValueAt("FrequencyAttribute"));

	// ## Custom update

	// ##
}

void CCPostProcessingSpecView::EventRefresh(Object* object)
{
	CCPostProcessingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCPostProcessingSpec*, object);
	SetStringValueAt("ShortDescription", editedObject->GetShortDescription());
	SetIntValueAt("InstanceNumber", editedObject->GetInstanceNumber());
	SetIntValueAt("NonEmptyCellNumber", editedObject->GetNonEmptyCellNumber());
	SetIntValueAt("CellNumber", editedObject->GetCellNumber());
	SetIntValueAt("MaxCellNumber", editedObject->GetMaxCellNumber());
	SetIntValueAt("MaxPreservedInformation", editedObject->GetMaxPreservedInformation());
	SetIntValueAt("TotalPartNumber", editedObject->GetTotalPartNumber());
	SetIntValueAt("MaxTotalPartNumber", editedObject->GetMaxTotalPartNumber());
	SetStringValueAt("FrequencyAttribute", editedObject->GetFrequencyAttribute());

	// ## Custom refresh

	// ##
}

const ALString CCPostProcessingSpecView::GetClassLabel() const
{
	return "Simplification parameters";
}

// ## Method implementation

void CCPostProcessingSpecView::SetPostProcessingParametersVisible(boolean bValue)
{
	CCPostProcessedAttributeArrayView* postProcessedAttributeArrayView;

	// Parametrage
	GetFieldAt("MaxCellNumber")->SetVisible(bValue);
	GetFieldAt("MaxPreservedInformation")->SetVisible(bValue);
	GetFieldAt("MaxTotalPartNumber")->SetVisible(bValue);
	postProcessedAttributeArrayView =
	    cast(CCPostProcessedAttributeArrayView*, GetFieldAt("PostProcessedAttributes"));
	postProcessedAttributeArrayView->GetFieldAt("MaxPartNumber")->SetVisible(bValue);
}

void CCPostProcessingSpecView::SetObject(Object* object)
{
	CCPostProcessingSpec* PostProcessingSpec;

	require(object != NULL);

	// Acces a l'objet edite
	PostProcessingSpec = cast(CCPostProcessingSpec*, object);

	// Parametrage du tableau des variables de coclustering
	cast(CCPostProcessedAttributeArrayView*, GetFieldAt("PostProcessedAttributes"))
	    ->SetObjectArray(PostProcessingSpec->GetPostProcessedAttributes());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

// ##
