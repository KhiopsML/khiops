// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2018-04-23 16:45:14
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "CCInstancesVariablesCoclusteringSpecView.h"

CCInstancesVariablesCoclusteringSpecView::CCInstancesVariablesCoclusteringSpecView()
{
	SetIdentifier("CCInstancesVariablesCoclusteringSpec");
	SetLabel("Instances Variables Coclustering parameters");

	// ## Custom constructor

	// CH IV Refactoring: nettoyer lignes suivantes? DDD?
	// DDD
	// pour pouvoir faire des benchmarks en activant/desactivant la pre ou la post optimisation
	// Ajout des sous-fiches
	if (GetLearningExpertMode())
		AddCardField("DataGridOptimizerParameters", "Data grid parameters",
			     new KWDataGridOptimizerParametersView);

	// Ajout d'une variable identifier qui donne le label des individus
	// Si une telle variable n'est pas indique, elle sera cree automatiquement avant l'analyse
	AddStringField("IdentifierAttribute", "Identifier variable", "");
	// Ajout d'un tableau des variables de coclustering generique
	KWAttributeAxisNameArrayView* attributeAxisNameArrayView;
	attributeAxisNameArrayView = new KWAttributeAxisNameArrayView;

	attributeAxisNameArrayView->SetMaxAxisNumber(
	    CCInstancesVariablesCoclusteringSpec::GetMaxCoclusteringAttributeNumber());
	attributeAxisNameArrayView->SetAttributeLabel("coclustering");
	AddListField("Attributes", "Coclustering variables axes", attributeAxisNameArrayView);

	// Personnalisation des action d'insertion et de supression
	attributeAxisNameArrayView->GetActionAt("InsertItemBefore")->SetVisible(false);
	attributeAxisNameArrayView->GetActionAt("InsertItemAfter")->SetStyle("Button");
	attributeAxisNameArrayView->GetActionAt("RemoveItem")->SetStyle("Button");
	attributeAxisNameArrayView->GetActionAt("InsertItemAfter")->SetLabel("Insert variable and axis");
	attributeAxisNameArrayView->GetActionAt("RemoveItem")->SetLabel("Remove variable and axis");

	// Info-bulles
	attributeAxisNameArrayView->GetFieldAt("Name")->SetHelpText(
	    "Input variable for the generic coclustering model."
	    "\n There must be at least two numerical or categorical or variable part input coclustering axes. Up to "
	    "ten axes are allowed.");
	attributeAxisNameArrayView->GetFieldAt("AxisName")
	    ->SetHelpText("Input axis for the generic coclustering model."
			  "\n There must be at least two numerical or categorical or variable part input coclustering "
			  "axes. Up to ten axes are allowed.");
	attributeAxisNameArrayView->GetActionAt("InsertItemAfter")
	    ->SetHelpText("Add a coclustering variable and its axis.");
	attributeAxisNameArrayView->GetActionAt("RemoveItem")
	    ->SetHelpText("Remove coclustering variable and its axis.");

	attributeAxisNameArrayView->SetVisible(false);

	GetFieldAt("IdentifierAttribute")
	    ->SetHelpText("Identifier variable (optional)."
			  "\n In case of a coclustering instances x variables, name of a variable that contains the "
			  "identifier of the records."
			  "\n Such a variable is automatically created for a instances x variables analysis if no name "
			  "is given ");
	// ##
}

CCInstancesVariablesCoclusteringSpecView::~CCInstancesVariablesCoclusteringSpecView()
{
	// ## Custom destructor

	// ##
}

CCInstancesVariablesCoclusteringSpec*
CCInstancesVariablesCoclusteringSpecView::GetCCInstancesVariablesCoclusteringSpec()
{
	require(objValue != NULL);
	return cast(CCInstancesVariablesCoclusteringSpec*, objValue);
}

void CCInstancesVariablesCoclusteringSpecView::EventUpdate(Object* object)
{
	CCInstancesVariablesCoclusteringSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCInstancesVariablesCoclusteringSpec*, object);
	editedObject->SetIdentifierAttribute(GetStringValueAt("IdentifierAttribute"));

	// ## Custom update

	// ##
}

void CCInstancesVariablesCoclusteringSpecView::EventRefresh(Object* object)
{
	CCInstancesVariablesCoclusteringSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCInstancesVariablesCoclusteringSpec*, object);
	SetStringValueAt("IdentifierAttribute", editedObject->GetIdentifierAttribute());

	// ## Custom refresh

	// ##
}

const ALString CCInstancesVariablesCoclusteringSpecView::GetClassLabel() const
{
	return "Instances Variables Coclustering parameters";
}

// ## Method implementation

void CCInstancesVariablesCoclusteringSpecView::SetObject(Object* object)
{
	CCInstancesVariablesCoclusteringSpec* coclusteringSpec;

	require(object != NULL);

	// Acces a l'objet edite
	coclusteringSpec = cast(CCInstancesVariablesCoclusteringSpec*, object);

	// Parametrage du tableau des axes et variables de coclustering generique
	cast(KWAttributeAxisNameArrayView*, GetFieldAt("Attributes"))
	    ->SetObjectArray(coclusteringSpec->GetAttributesAndAxes());

	// CH IV Refactoring: nettoyer lignes suivantes? DDD?
	// CH AB DDD
	// Parametrage des sous-fiches
	if (GetLearningExpertMode())
		cast(KWDataGridOptimizerParametersView*, GetFieldAt("DataGridOptimizerParameters"))
		    ->SetObject(coclusteringSpec->GetOptimizationParameters());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

// ##
