// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCCoclusteringSpecView.h"

CCCoclusteringSpecView::CCCoclusteringSpecView()
{
	SetIdentifier("CCCoclusteringSpec");
	SetLabel("Coclustering parameters");
	AddStringField("FrequencyAttribute", "Frequency variable", "");

	// ## Custom constructor

	// Ajout d'un tableau des variables de coclustering
	KWAttributeNameArrayView* attributeNameArrayView;
	attributeNameArrayView = new KWAttributeNameArrayView;
	attributeNameArrayView->SetMaxAttributeNumber(CCCoclusteringSpec::GetMaxCoclusteringAttributeNumber());
	attributeNameArrayView->SetAttributeLabel("coclustering");
	AddListField("Attributes", "Coclustering variables", attributeNameArrayView);

	// Personnalisation des actions d'insertion et de supression
	attributeNameArrayView->GetActionAt("InsertItemBefore")->SetVisible(false);
	attributeNameArrayView->GetActionAt("InsertItemAfter")->SetStyle("Button");
	attributeNameArrayView->GetActionAt("RemoveItem")->SetStyle("Button");
	attributeNameArrayView->GetActionAt("InsertItemAfter")->SetLabel("Insert variable");
	attributeNameArrayView->GetActionAt("RemoveItem")->SetLabel("Remove variable");

	// Le tableau des variables est affiche avant la variable d'effectif
	MoveFieldBefore("Attributes", "FrequencyAttribute");

	// Info-bulles
	attributeNameArrayView->GetFieldAt("Name")->SetHelpText(
	    "Input variable for the coclustering model."
	    "\n There must be at least two numerical or categorical input coclustering variables. Up to ten variables "
	    "are allowed.");
	attributeNameArrayView->GetActionAt("InsertItemAfter")->SetHelpText("Add a coclustering variable.");
	attributeNameArrayView->GetActionAt("RemoveItem")->SetHelpText("Remove coclustering variable.");
	GetFieldAt("FrequencyAttribute")
	    ->SetHelpText(
		"Frequency variable (optional)."
		"\n Name of a variable that contains the frequency of the records."
		"\n Using the frequency variable is equivalent to duplicating the records in the input database,"
		"\n where the number of duplicates per record is equal to the frequency.");

	// Short cuts
	attributeNameArrayView->GetActionAt("InsertItemAfter")->SetShortCut('I');
	attributeNameArrayView->GetActionAt("RemoveItem")->SetShortCut('M');

	// ##
}

CCCoclusteringSpecView::~CCCoclusteringSpecView()
{
	// ## Custom destructor

	// ##
}

CCCoclusteringSpec* CCCoclusteringSpecView::GetCCCoclusteringSpec()
{
	require(objValue != NULL);
	return cast(CCCoclusteringSpec*, objValue);
}

void CCCoclusteringSpecView::EventUpdate(Object* object)
{
	CCCoclusteringSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCCoclusteringSpec*, object);
	editedObject->SetFrequencyAttribute(GetStringValueAt("FrequencyAttribute"));

	// ## Custom update

	// ##
}

void CCCoclusteringSpecView::EventRefresh(Object* object)
{
	CCCoclusteringSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCCoclusteringSpec*, object);
	SetStringValueAt("FrequencyAttribute", editedObject->GetFrequencyAttribute());

	// ## Custom refresh

	// ##
}

const ALString CCCoclusteringSpecView::GetClassLabel() const
{
	return "Coclustering parameters";
}

// ## Method implementation

void CCCoclusteringSpecView::SetObject(Object* object)
{
	CCCoclusteringSpec* coclusteringSpec;

	require(object != NULL);

	// Acces a l'objet edite
	coclusteringSpec = cast(CCCoclusteringSpec*, object);

	// Parametrage du tableau des variables de coclustering
	cast(KWAttributeNameArrayView*, GetFieldAt("Attributes"))->SetObjectArray(coclusteringSpec->GetAttributes());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

// ##