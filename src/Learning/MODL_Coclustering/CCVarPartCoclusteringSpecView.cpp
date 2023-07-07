// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCVarPartCoclusteringSpecView.h"

CCVarPartCoclusteringSpecView::CCVarPartCoclusteringSpecView()
{
	SetIdentifier("CCVarPartCoclusteringSpec");
	SetLabel("Instances Variables coclustering parameters");
	AddStringField("IdentifierAttribute", "Identifier variable", "");

	// ## Custom constructor

	// CH IV Refactoring: nettoyer lignes suivantes? DDD?
	// DDD
	// pour pouvoir faire des benchmarks en activant/desactivant la pre ou la post optimisation
	// Ajout des sous-fiches
	if (GetLearningExpertMode())
		AddCardField("DataGridOptimizerParameters", "Data grid parameters",
			     new KWDataGridOptimizerParametersView);

	// Ajout d'un tableau des variables de coclustering VarPart
	KWAttributeAxisNameArrayView* attributeAxisNameArrayView;
	attributeAxisNameArrayView = new KWAttributeAxisNameArrayView;

	attributeAxisNameArrayView->SetMaxAxisNumber(CCVarPartCoclusteringSpec::GetMaxCoclusteringAttributeNumber());
	attributeAxisNameArrayView->SetAttributeLabel("coclustering");
	AddListField("Attributes", "Coclustering variables axes", attributeAxisNameArrayView);

	// Personnalisation des action d'insertion et de supression
	attributeAxisNameArrayView->GetActionAt("InsertItemBefore")->SetVisible(false);
	attributeAxisNameArrayView->GetActionAt("InsertItemAfter")->SetStyle("Button");
	attributeAxisNameArrayView->GetActionAt("RemoveItem")->SetStyle("Button");
	attributeAxisNameArrayView->GetActionAt("InsertItemAfter")->SetLabel("Insert variable and axis");
	attributeAxisNameArrayView->GetActionAt("RemoveItem")->SetLabel("Remove variable and axis");

	// Info-bulles
	// CH IV Refactoring: deplacer les info-bulles vers KWAttributeAxisNameView?
	attributeAxisNameArrayView->GetFieldAt("AttributeName")
	    ->SetHelpText("Input variable for the VarPart coclustering model."
			  "\n There must be at least two numerical or categorical or VarPart input coclustering axes. "
			  "Up to ten axes are allowed.");
	attributeAxisNameArrayView->GetFieldAt("OwnerAttributeName")
	    ->SetHelpText("Input axis for the VarPart coclustering model."
			  "\n There must be at least two numerical or categorical or variable part input coclustering "
			  "axes. Up to ten axes are allowed.");
	attributeAxisNameArrayView->GetActionAt("InsertItemAfter")
	    ->SetHelpText("Add a coclustering variable and its axis.");
	attributeAxisNameArrayView->GetActionAt("RemoveItem")
	    ->SetHelpText("Remove coclustering variable and its axis.");

	// CH IV Refactoring: est-ce toujours utile de pouvoir parametrer le tableau d'attributs
	// CH IV Refactoring: les classes de type KWAttributeAxisName* sont-elles toujours utilisees
	attributeAxisNameArrayView->SetVisible(false);

	GetFieldAt("IdentifierAttribute")
	    ->SetHelpText(
		"Identifier variable (optional)."
		"\n In the case of instances x variables coclustering, name of a variable that contains the identifier "
		"of the records."
		"\n Such a variable is automatically created for instances x variables analysis if no name is given ");
	// ##
}

CCVarPartCoclusteringSpecView::~CCVarPartCoclusteringSpecView()
{
	// ## Custom destructor

	// ##
}

CCVarPartCoclusteringSpec* CCVarPartCoclusteringSpecView::GetCCVarPartCoclusteringSpec()
{
	require(objValue != NULL);
	return cast(CCVarPartCoclusteringSpec*, objValue);
}

void CCVarPartCoclusteringSpecView::EventUpdate(Object* object)
{
	CCVarPartCoclusteringSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCVarPartCoclusteringSpec*, object);
	editedObject->SetIdentifierAttribute(GetStringValueAt("IdentifierAttribute"));

	// ## Custom update

	// ##
}

void CCVarPartCoclusteringSpecView::EventRefresh(Object* object)
{
	CCVarPartCoclusteringSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCVarPartCoclusteringSpec*, object);
	SetStringValueAt("IdentifierAttribute", editedObject->GetIdentifierAttribute());

	// ## Custom refresh

	// ##
}

const ALString CCVarPartCoclusteringSpecView::GetClassLabel() const
{
	return "Instances Variables coclustering parameters";
}

// ## Method implementation

void CCVarPartCoclusteringSpecView::SetObject(Object* object)
{
	CCVarPartCoclusteringSpec* coclusteringSpec;

	require(object != NULL);

	// Acces a l'objet edite
	coclusteringSpec = cast(CCVarPartCoclusteringSpec*, object);

	// Parametrage du tableau des axes et variables de coclustering VarPart
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
